#include "extras/host/ethernet5500/host_socket_info.h"

// TODO(jamessynge): Use poll() or epoll() to discover changes of state of the
// sockets. Maybe consider emulating a hardware socket more thoroughly.

#include <asm-generic/errno.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdint.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <map>
#include <set>
#include <string>
#include <utility>

#include "absl/flags/flag.h"
#include "absl/flags/marshalling.h"
#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"
#include "mcucore/extras/host/posix_errno.h"

namespace {
struct PortMap {
  uint16_t Lookup(uint16_t port) const {
    const auto it = map.find(port);
    if (it == map.end()) {
      return port;
    } else {
      return map.find(port)->second;
    }
  }

  std::map<uint16_t, uint16_t> map;
};

std::string AbslUnparseFlag(const PortMap& port_mappings) {
  return absl::StrJoin(port_mappings.map, ",", absl::PairFormatter("="));
}

bool AbslParseFlag(absl::string_view text, PortMap* dst, std::string* error) {
  for (const absl::string_view entry :
       absl::StrSplit(text, ',', absl::SkipEmpty())) {
    std::pair<absl::string_view, absl::string_view> ports =
        absl::StrSplit(entry, absl::MaxSplits('=', 1));
    uint16_t port1, port2;
    if (!absl::ParseFlag(ports.first, &port1, error) ||
        !absl::ParseFlag(ports.second, &port2, error)) {
      return false;
    }
    if (!dst->map.try_emplace(port1, port2).second) {
      absl::StrAppend(error, "Port ", port1, " is duplicated.");
      return false;
    }
  }
  return true;
}

std::string PortsToString(uint16_t tcp_port, uint16_t mapped_tcp_port) {
  if (tcp_port == mapped_tcp_port) {
    return absl::StrCat(tcp_port);
  } else {
    return absl::StrCat(tcp_port, " (mapped to ", mapped_tcp_port, ")");
  }
}

bool HaveFd(int fd) { return fd >= 0; }

}  // namespace

ABSL_FLAG(PortMap, tcp_server_port_map, (PortMap{}),
          "Mapping from requested port to actually listened to port; a comma "
          "separated list of port=port pairs, where the first is the requested "
          "port and the second is the port to which it is mapped. This helps "
          "avoid issues with privileged ports, e.g. those below 1024.");

namespace mcunet_host {

HostSocketInfo::HostSocketInfo(uint8_t sock_num) : sock_num_(sock_num) {
  VLOG(1) << "Create HostSocketInfo for socket " << sock_num_;
}

HostSocketInfo::~HostSocketInfo() {
  CloseConnectionSocket();
  CloseListenerSocket();
}

std::string HostSocketInfo::ToString() const {
  std::string result = absl::StrCat("{sock=", sock_num_);
  if (HaveFd(connection_socket_fd_)) {
    absl::StrAppend(&result, ", connection_fd=", connection_socket_fd_);
  }
  if (HaveFd(listener_socket_fd_)) {
    absl::StrAppend(&result, ", listener_fd=", listener_socket_fd_);
  }
  if (tcp_port_ != 0 || mapped_tcp_port_ != 0) {
    absl::StrAppend(&result,
                    ", tcp_port=", PortsToString(tcp_port_, mapped_tcp_port_));
  }
  absl::StrAppend(&result, "}");
  return result;
}

bool HostSocketInfo::IsUnused() {
  VLOG(4) << "HostSocketInfo::IsUnused " << ToString();
  if (HaveFd(listener_socket_fd_)) {
    CHECK_GT(tcp_port_, 0) << ToString();
    CHECK_GT(mapped_tcp_port_, 0) << ToString();
    CHECK(!HaveFd(connection_socket_fd_)) << ToString();
    return false;
  }
  CHECK_EQ(tcp_port_, 0) << ToString();
  CHECK_EQ(mapped_tcp_port_, 0) << ToString();
  if (HaveFd(connection_socket_fd_)) {
    return false;
  }
  return true;
}

uint16_t HostSocketInfo::IsTcpListener() {
  if (HaveFd(listener_socket_fd_)) {
    CHECK_GT(tcp_port_, 0);
    return tcp_port_;
  } else {
    CHECK_EQ(tcp_port_, 0);
    return 0;
  }
}

bool HostSocketInfo::IsConnected() { return HaveFd(connection_socket_fd_); }

bool HostSocketInfo::IsConnectionHalfClosed() {
  if (!HaveFd(connection_socket_fd_)) {
    LOG(WARNING) << "Socket " << sock_num_
                 << " isn't even open, can't be half closed.";
    return false;
  }
  if (!can_read_from_connection_) {
    return true;
  }

  // See if we can peek at the next byte.
  char c;
  int size = recv(connection_socket_fd_, &c, 1, MSG_PEEK | MSG_DONTWAIT);
  const auto error_number = errno;
  VLOG(2) << "IsConnectionHalfClosed: recv from " << ToString() << " -> "
          << size
          << (size >= 0 ? std::string()
                        : absl::StrCat("\nWith ", mcucore_host::ErrnoToString(
                                                      error_number)));
  if (size == 1) {
    // There is data available to read right now, so not half-closed from our
    // perspective.
    return false;
  } else if (size == 0) {
    // The connection may be half-closed, may be shutdown or disconnected.
    can_read_from_connection_ = false;
    return true;
  }
  CHECK(false) << "recv from " << ToString() << " -> " << size
               << (size >= 0 ? std::string()
                             : absl::StrCat(
                                   "\nWith ",
                                   mcucore_host::ErrnoToString(error_number)));
  return true;
}

bool HostSocketInfo::CanReadFromConnection() {
  if (!HaveFd(connection_socket_fd_)) {
    LOG(ERROR) << "Socket " << sock_num_ << " isn't open.";
    return false;
  }
  if (can_read_from_connection_ && Peek() >= 0) {
    return true;
  }
  return false;
}

bool HostSocketInfo::IsClosed() {
  return !HaveFd(listener_socket_fd_) && !HaveFd(connection_socket_fd_);
}

uint8_t HostSocketInfo::SocketStatus() {
  // For convenience, I'm returning values matching those used by the W5500
  // chip, but long term I want to eliminate this method, or define my own
  // status *mask*.
  if (IsUnused()) {
    return kStatusClosed;
  } else if (IsTcpListener() && !AcceptConnection()) {
    return kStatusListening;
  } else if (IsConnected()) {
    if (IsConnectionHalfClosed()) {
      return kStatusCloseWait;
    }
    return kStatusEstablished;
  }
  LOG(DFATAL) << "Unsupported status of socket " << sock_num_;
  return kStatusClosed;
}

////////////////////////////////////////////////////////////////////////////////
// Methods modifying sockets.

bool HostSocketInfo::InitializeTcpListener(const uint16_t new_tcp_port) {
  const uint16_t mapped_tcp_port =
      absl::GetFlag(FLAGS_tcp_server_port_map).Lookup(new_tcp_port);

  VLOG(1) << "InitializeTcpListener on port "
          << PortsToString(new_tcp_port, mapped_tcp_port) << " for socket "
          << sock_num_;
  CloseConnectionSocket();
  if (HaveFd(listener_socket_fd_)) {
    if (tcp_port_ == new_tcp_port) {
      return true;
    }
    CloseListenerSocket();
  }
  listener_socket_fd_ = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (!HaveFd(listener_socket_fd_)) {
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
  addr.sin_port = htons(mapped_tcp_port);
  if (::bind(listener_socket_fd_, reinterpret_cast<sockaddr*>(&addr),
             sizeof addr) < 0) {
    const auto error_number = errno;
    LOG(ERROR) << "Unable to set bind socket " << sock_num_ << " to INADDR_ANY:"
               << PortsToString(new_tcp_port, mapped_tcp_port) << ", "
               << mcucore_host::ErrnoToString(error_number);
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
  mapped_tcp_port_ = mapped_tcp_port;
  VLOG(1) << "Socket " << sock_num_ << " (fd " << listener_socket_fd_
          << ") is now listening for connections to port "
          << PortsToString(new_tcp_port, mapped_tcp_port);
  return true;
}

bool HostSocketInfo::AcceptConnection() {
  DCHECK_LT(connection_socket_fd_, 0);
  VLOG(4) << "AcceptConnection for socket " << sock_num_;
  if (HaveFd(listener_socket_fd_)) {
    sockaddr_in addr;
    socklen_t addrlen = sizeof addr;
    connection_socket_fd_ = ::accept(
        listener_socket_fd_, reinterpret_cast<sockaddr*>(&addr), &addrlen);
    if (HaveFd(connection_socket_fd_)) {
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
      can_write_to_connection_ = can_read_from_connection_ = true;
      return true;
    }
    const int error_number = errno;
    VLOG(4) << "accept for socket " << sock_num_ << " failed with "
            << mcucore_host::ErrnoToString(error_number);
  }
  return false;
}

bool HostSocketInfo::DisconnectConnectionSocket() {
  if (HaveFd(connection_socket_fd_)) {
    VLOG(1) << "Disconnecting connection (" << connection_socket_fd_
            << ") for socket " << sock_num_;
    if (::shutdown(connection_socket_fd_, SHUT_WR) == 0) {
      return true;
    }
  }
  return false;
}

void HostSocketInfo::CloseConnectionSocket() {
  if (HaveFd(connection_socket_fd_)) {
    VLOG(1) << "Closing connection (" << connection_socket_fd_
            << ") for socket " << sock_num_;
    ::close(connection_socket_fd_);
  }
  connection_socket_fd_ = -1;
  can_read_from_connection_ = false;
  can_write_to_connection_ = false;
}

void HostSocketInfo::CloseListenerSocket() {
  if (HaveFd(listener_socket_fd_)) {
    VLOG(1) << "Closing listener (" << connection_socket_fd_ << ") for socket "
            << sock_num_;
    ::close(listener_socket_fd_);
  }
  listener_socket_fd_ = -1;
  tcp_port_ = 0;
  mapped_tcp_port_ = 0;
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
  if (!HaveFd(connection_socket_fd_)) {
    LOG(WARNING) << "Socket doesn't have an open connection.";
    return -1;
  }
  return ::send(connection_socket_fd_, buf, len, 0);
}

void HostSocketInfo::Flush() {
  if (!HaveFd(connection_socket_fd_)) {
    LOG(WARNING) << "Socket doesn't have an open connection.";
  }
  // There is no linux method for flushing a socket.
}

ssize_t HostSocketInfo::AvailableBytes() {
  // There isn't a portable way to determine the number bytes available for
  // reading, so we peek and see if we can read at least a fixed number of
  // bytes, a number that is larger than a small embedded system is likely to
  // have available in a single buffer.
  uint8_t buffer[1000];
  const ssize_t size = RecvInternal(buffer, sizeof(buffer), /*peek=*/true);
  if (size >= 0) {
    return size;
  }
  if (can_read_from_connection_ || can_write_to_connection_) {
    return 0;
  }
  return -1;
}

int HostSocketInfo::Peek() {
  uint8_t b;
  const ssize_t size = RecvInternal(&b, 1, /*peek=*/false);
  if (size == 1) {
    return b;
  } else {
    return -1;
  }
}

ssize_t HostSocketInfo::Recv(uint8_t* buf, size_t len) {
  return RecvInternal(buf, len, /*peek=*/false);
}

ssize_t HostSocketInfo::RecvInternal(uint8_t* buf, size_t len, bool peek) {
  if (!HaveFd(connection_socket_fd_)) {
    LOG(WARNING) << "Socket doesn't have an open connection.";
    return -1;
  }
  if (!can_read_from_connection_) {
    return -1;
  }

  const ssize_t size = recv(connection_socket_fd_, buf, len,
                            MSG_DONTWAIT | (peek ? MSG_PEEK : 0));
  const auto error_number = errno;
  if (size > 0) {
    return size;
  }
  if (size == 0) {
    VLOG(2)
        << "Detected unreadable socket in HostSocketInfo::RecvInternal from "
        << ToString();
    can_read_from_connection_ = false;
    return 0;
  }

  switch (error_number) {
#ifdef EAGAIN
    case EAGAIN:
#endif
#if defined(EWOULDBLOCK) && (!defined(EAGAIN) || (EWOULDBLOCK != EAGAIN))
    case EWOULDBLOCK:
#endif
    case EINTR:
      // Try again later.
      VLOG(2) << "HostSocketInfo::RecvInternal from " << ToString()
              << " need to try again later; "
              << mcucore_host::ErrnoToString(errno);
      return -1;

    case ECONNRESET:
    case ECONNABORTED:
    case ENOTCONN:
      // Seems the connection is broken.
      VLOG(2) << "HostSocketInfo::RecvInternal from " << ToString()
              << " failed with " << mcucore_host::ErrnoToString(errno);
      CloseConnectionSocket();
      return -1;

    default:
      CHECK(false) << "HostSocketInfo::RecvInternal from " << ToString()
                   << " failed with unexpected error; "
                   << mcucore_host::ErrnoToString(errno);
  }
  return -1;
}

}  // namespace mcunet_host
