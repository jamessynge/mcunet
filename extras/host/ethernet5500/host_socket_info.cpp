#include "extras/host/ethernet5500/host_socket_info.h"

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdint.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>

#include "absl/log/check.h"
#include "absl/log/log.h"

namespace mcunet_host {

HostSocketInfo::HostSocketInfo(uint8_t sock_num) : sock_num_(sock_num) {
  VLOG(1) << "Create HostSocketInfo for socket " << sock_num_;
}

HostSocketInfo::~HostSocketInfo() {
  CloseConnectionSocket();
  CloseListenerSocket();
}

bool HostSocketInfo::IsUnused() {
  VLOG(1) << "HostSocketInfo::IsUnused " << sock_num_ << ", "
          << listener_socket_fd_ << ", " << tcp_port_ << ", "
          << connection_socket_fd_;
  return listener_socket_fd_ < 0 && connection_socket_fd_ < 0;
}

uint16_t HostSocketInfo::IsTcpListener() {
  if (listener_socket_fd_ >= 0) {
    CHECK_GT(tcp_port_, 0);
    return tcp_port_;
  } else {
    CHECK_EQ(tcp_port_, 0);
    return 0;
  }
}

bool HostSocketInfo::IsConnected() { return connection_socket_fd_ >= 0; }

bool HostSocketInfo::IsConnectionHalfClosed() {
  if (connection_socket_fd_ < 0) {
    LOG(WARNING) << "Socket " << sock_num_
                 << " isn't even open, can't be half closed.";
    return false;
  } else {
    // See if we can peek at the next byte.
    char c;
    return 0 == recv(connection_socket_fd_, &c, 1, MSG_PEEK);
  }
}

bool HostSocketInfo::CanReadFromConnection() {
  if (connection_socket_fd_ < 0) {
    LOG(ERROR) << "Socket " << sock_num_ << " isn't open.";
    return false;
  } else {
    while (true) {
      // See if we can peek at the next byte.
      char c;
      const auto ret =
          recv(connection_socket_fd_, &c, 1, MSG_PEEK | MSG_DONTWAIT);
      const auto error_number = errno;
      DVLOG(1) << "recv -> " << ret;
      if (ret > 0) {
        CHECK_EQ(ret, 1);
        // There is data available for reading.
        return true;
      } else if (ret < 0) {
        DVLOG(1) << "errno " << error_number << ": "
                 << std::strerror(error_number);
        if (error_number == EINTR) {
          // We were interrupted, so try again.
          continue;

        } else if (error_number == EAGAIN || error_number == EWOULDBLOCK) {
          // Reading would block, so peer must NOT have called shutdown with
          // how==SHUT_WR.
          return true;
        } else {
          // Some other error (invalid socket, etc.). Assume that this means
          // we can't read from the connection.
          return false;
        }
      } else {
        // ret == 0: for a TCP socket, this means that the peer has shutdown
        // the socket, at least for writing.
        return false;
      }
    }
  }
}

bool HostSocketInfo::IsClosed() {
  return listener_socket_fd_ < 0 && connection_socket_fd_ < 0;
}

uint8_t HostSocketInfo::SocketStatus() {
  // For convenience, I'm returning values matching those used by the W5500
  // chip, but long term I want to eliminate this method, or define my own
  // status *mask*.
  if (IsUnused()) {
    return kStatusClosed;
  } else if (IsTcpListener()) {
    return kStatusListening;
  } else if (IsConnectionHalfClosed()) {
    return kStatusCloseWait;
  } else if (IsConnected()) {
    return kStatusEstablished;
  }
  LOG(DFATAL) << "Unsupported status of socket " << sock_num_;
  return kStatusClosed;
}

////////////////////////////////////////////////////////////////////////////////
// Methods modifying sockets.

bool HostSocketInfo::InitializeTcpListener(uint16_t new_tcp_port) {
  VLOG(1) << "InitializeTcpListener on port " << new_tcp_port << " for socket "
          << sock_num_;
  CloseConnectionSocket();
  if (listener_socket_fd_ >= 0) {
    if (tcp_port_ == new_tcp_port) {
      return true;
    }
    CloseListenerSocket();
  }
  listener_socket_fd_ = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (listener_socket_fd_ < 0) {
    LOG(ERROR) << "Unable to create listener for socket " << sock_num_;
    return false;
  }
  int value = 1;
  socklen_t len = sizeof(value);
  if (::setsockopt(listener_socket_fd_, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                   &value, len) < 0) {
    LOG(ERROR) << "Unable to set REUSEADDR for socket " << sock_num_;
    CloseListenerSocket();
    return false;
  }
  sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(new_tcp_port);
  if (::bind(listener_socket_fd_, reinterpret_cast<sockaddr*>(&addr),
             sizeof addr) < 0) {
    auto error_number = errno;
    LOG(ERROR) << "Unable to set bind socket " << sock_num_
               << " to INADDR_ANY:" << new_tcp_port
               << ", error message: " << std::strerror(error_number);
    CloseListenerSocket();
    return false;
  }
  if (!SetNonBlocking(listener_socket_fd_)) {
    LOG(ERROR) << "Unable to make listener non-blocking for socket "
               << sock_num_;
    return false;
  }
  if (::listen(listener_socket_fd_, 9) < 0) {
    CloseListenerSocket();
    return false;
  }
  tcp_port_ = new_tcp_port;
  VLOG(1) << "Socket " << sock_num_ << " (fd " << listener_socket_fd_
          << ") is now listening for connections to port " << tcp_port_;
  return true;
}

bool HostSocketInfo::AcceptConnection() {
  DCHECK_LT(connection_socket_fd_, 0);
  VLOG(2) << "AcceptConnection for socket " << sock_num_;
  if (listener_socket_fd_ >= 0) {
    sockaddr_in addr;
    socklen_t addrlen = sizeof addr;
    connection_socket_fd_ = ::accept(
        listener_socket_fd_, reinterpret_cast<sockaddr*>(&addr), &addrlen);
    if (connection_socket_fd_ >= 0) {
      VLOG(1) << "Accepted a connection for socket " << sock_num_ << " with fd "
              << connection_socket_fd_;

      // We leave the connection as blocking so that Send can be blocking, while
      // Recv can be non-blocking by passing MSG_DONTWAIT.
      // if (!SetNonBlocking(listener_socket_fd_)) {
      //   LOG(WARNING) << "Unable to make connection non-blocking for socket "
      //                << sock_num_;
      // }
      // The W5500 doesn't keep track of that fact that the socket used to be
      // listening, so to be a better emulation of its behavior, we now close
      // the listener socket, and will re-open it later if requested.
      CloseListenerSocket();
      return true;
    }
    int error_number = errno;
    VLOG(3) << "accept for socket " << sock_num_
            << " failed with error message: " << std::strerror(error_number);
  }
  return false;
}

bool HostSocketInfo::DisconnectConnectionSocket() {
  if (connection_socket_fd_ >= 0) {
    VLOG(1) << "Disconnecting connection (" << connection_socket_fd_
            << ") for socket " << sock_num_;
    if (::shutdown(connection_socket_fd_, SHUT_WR) == 0) {
      return true;
    }
  }
  return false;
}

void HostSocketInfo::CloseConnectionSocket() {
  if (connection_socket_fd_ >= 0) {
    VLOG(1) << "Closing connection (" << connection_socket_fd_
            << ") for socket " << sock_num_;
    ::close(connection_socket_fd_);
  }
  connection_socket_fd_ = -1;
}

void HostSocketInfo::CloseListenerSocket() {
  if (listener_socket_fd_ >= 0) {
    VLOG(1) << "Closing listener (" << connection_socket_fd_ << ") for socket "
            << sock_num_;
    ::close(listener_socket_fd_);
  }
  listener_socket_fd_ = -1;
  tcp_port_ = 0;
}

// static
bool HostSocketInfo::SetNonBlocking(int fd) {
  int fcntl_return = fcntl(fd, F_GETFL, 0);
  if (fcntl_return < 0) {
    auto msg = std::strerror(errno);
    LOG(ERROR) << "Failed to get file status flags for fd " << fd
               << ", error message: " << msg;
    return false;
  }
  if (fcntl_return & O_NONBLOCK) {
    return true;
  }
  fcntl_return = fcntl(fd, F_SETFL, fcntl_return | O_NONBLOCK);
  if (fcntl_return < 0) {
    auto msg = std::strerror(errno);
    LOG(ERROR) << "Failed to set file status flags for fd " << fd
               << ", error message: " << msg;
    return false;
  }
  return true;
}

////////////////////////////////////////////////////////////////////////////////
// Methods using open sockets.

ssize_t HostSocketInfo::Send(const uint8_t* buf, size_t len) {
  if (connection_socket_fd_ < 0) {
    LOG(WARNING) << "Socket doesn't have an open connection.";
    return -1;
  }
  return ::send(connection_socket_fd_, buf, len, 0);
}

void HostSocketInfo::Flush() {
  if (connection_socket_fd_ < 0) {
    LOG(WARNING) << "Socket doesn't have an open connection.";
  }
  // There is no linux method for flushing a socket.
}

ssize_t HostSocketInfo::AvailableBytes() {
  if (connection_socket_fd_ < 0) {
    LOG(WARNING) << "Socket doesn't have an open connection.";
    return -1;
  }
  // There isn't a portable way to determine the number bytes available for
  // reading, so we peek and see if we can read at least a fixed number of
  // bytes, a number that is larger than a small embedded system is likely to
  // have available in a single buffer.
  uint8_t buffer[1000];
  return ::recv(connection_socket_fd_, buffer, sizeof buffer,
                MSG_DONTWAIT | MSG_PEEK);
}

int HostSocketInfo::Peek() {
  if (connection_socket_fd_ < 0) {
    LOG(WARNING) << "Socket doesn't have an open connection.";
    return -1;
  }
  // There isn't a portable way to determine the number bytes available for
  // reading, so we peek and see if we can read at least a fixed number of
  // bytes, a number that is larger than a small embedded system is likely to
  // have available in a single buffer.
  uint8_t value;
  if (::recv(connection_socket_fd_, &value, 1, MSG_DONTWAIT | MSG_PEEK) == 1) {
    return value;
  } else {
    return -1;
  }
}

ssize_t HostSocketInfo::Recv(uint8_t* buf, size_t len) {
  if (connection_socket_fd_ < 0) {
    LOG(WARNING) << "Socket doesn't have an open connection.";
    return -1;
  }
  return ::recv(connection_socket_fd_, buf, len, MSG_DONTWAIT);
}

}  // namespace mcunet_host
