#include "extras/host/ethernet5500/host_network.h"

#include <errno.h>
#include <netinet/in.h>
#include <stddef.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <map>
#include <memory>
#include <string_view>

#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/strings/str_cat.h"
#include "extras/host/ethernet5500/host_socket_info.h"

namespace mcunet_host {

static constexpr int kMaxSockets = 8;

struct HostNetworkImpl {
  HostSocketInfo *GetHostSocketInfo(const uint8_t sock_num) {
    if (!(0 <= sock_num && sock_num <= kMaxSockets)) {
      LOG(ERROR) << "Invalid socket number: " << static_cast<int>(sock_num);
      return nullptr;
    }
    CHECK(sockets.find(sock_num) != sockets.end())
        << "Socket " << static_cast<int>(sock_num) << " is missing";
    CHECK(sockets[sock_num].get() != nullptr)
        << "Socket " << static_cast<int>(sock_num) << " is missing";
    return sockets[sock_num].get();
  }

  std::map<uint8_t, std::unique_ptr<HostSocketInfo>> sockets;
};

HostNetwork::HostNetwork() : impl_(std::make_unique<HostNetworkImpl>()) {
  for (int sock_num = 0; sock_num < kMaxSockets; ++sock_num) {
    impl_->sockets[sock_num] = std::make_unique<HostSocketInfo>(sock_num);
  }
}

HostNetwork::~HostNetwork() {}

////////////////////////////////////////////////////////////////////////////////
// Methods getting the status of a socket.

int HostNetwork::FindUnusedSocket() {
  for (int sock_num = 0; sock_num < kMaxSockets; ++sock_num) {
    auto *info = impl_->GetHostSocketInfo(sock_num);
    if (info != nullptr && info->IsUnused()) {
      VLOG(2) << "HostNetwork::FindUnusedSocket found " << sock_num;
      return sock_num;
    }
  }
  VLOG(2) << "HostNetwork::FindUnusedSocket found no unused sockets";
  return -1;
}

uint16_t HostNetwork::SocketIsTcpListener(uint8_t sock_num) {
  uint16_t port = 0;
  auto *info = impl_->GetHostSocketInfo(sock_num);
  if (info != nullptr) {
    port = info->IsTcpListener();
  }
  VLOG(2) << "HostNetwork::SocketIsTcpListener -> " << port;
  return port;
}

bool HostNetwork::SocketIsInTcpConnectionLifecycle(uint8_t sock_num) {
  auto *info = impl_->GetHostSocketInfo(sock_num);
  // This doesn't quite cover the case where the socket is either being
  // established or is being closed (i.e. in TIME_WAIT state).
  bool result = info != nullptr && info->IsConnected();
  VLOG(2) << "HostNetwork::SocketIsInTcpConnectionLifecycle -> "
          << (result ? "true" : "false");
  return result;
}

bool HostNetwork::SocketIsHalfClosed(uint8_t sock_num) {
  auto *info = impl_->GetHostSocketInfo(sock_num);
  bool result = info != nullptr && info->IsConnectionHalfClosed();
  VLOG(2) << "HostNetwork::SocketIsHalfClosed -> "
          << (result ? "true" : "false");
  return result;
}

bool HostNetwork::SocketIsClosed(uint8_t sock_num) {
  auto *info = impl_->GetHostSocketInfo(sock_num);
  bool result = info != nullptr && info->IsClosed();
  VLOG(2) << "HostNetwork::SocketIsClosed -> " << (result ? "true" : "false");
  return result;
}

uint8_t HostNetwork::SocketStatus(uint8_t sock_num) {
  // For convenience, I'm returning values matching those used by the W5500
  // chip, but long term I want to eliminate this method, or define my own
  // status *mask*.
  auto *info = impl_->GetHostSocketInfo(sock_num);
  if (info == nullptr) {
    return HostSocketInfo::kStatusClosed;
  } else {
    return info->SocketStatus();
  }
}

////////////////////////////////////////////////////////////////////////////////
// Methods modifying sockets.

bool HostNetwork::InitializeTcpListenerSocket(uint8_t sock_num,
                                              uint16_t tcp_port) {
  auto *info = impl_->GetHostSocketInfo(sock_num);
  return info != nullptr && info->InitializeTcpListener(tcp_port);
}

bool HostNetwork::AcceptConnection(uint8_t sock_num) {
  auto *info = impl_->GetHostSocketInfo(sock_num);
  return info != nullptr && info->AcceptConnection();
}

bool HostNetwork::DisconnectSocket(uint8_t sock_num) {
  auto *info = impl_->GetHostSocketInfo(sock_num);
  return info != nullptr && info->DisconnectConnectionSocket();
}

bool HostNetwork::CloseSocket(uint8_t sock_num) {
  auto *info = impl_->GetHostSocketInfo(sock_num);
  if (info != nullptr) {
    info->CloseConnectionSocket();
    info->CloseListenerSocket();
    return true;
  }
  return false;
}

////////////////////////////////////////////////////////////////////////////////
// Methods using open sockets.

ssize_t HostNetwork::Send(uint8_t sock_num, const uint8_t *buf, size_t len) {
  auto *info = impl_->GetHostSocketInfo(sock_num);
  if (info != nullptr) {
    return info->Send(buf, len);
  } else {
    return -1;
  }
}

void HostNetwork::Flush(uint8_t sock_num) {
  auto *info = impl_->GetHostSocketInfo(sock_num);
  if (info != nullptr) {
    return info->Flush();
  }
}

ssize_t HostNetwork::AvailableBytes(uint8_t sock_num) {
  auto *info = impl_->GetHostSocketInfo(sock_num);
  if (info != nullptr) {
    return info->AvailableBytes();
  } else {
    return -1;
  }
}

int HostNetwork::Peek(uint8_t sock_num) {
  auto *info = impl_->GetHostSocketInfo(sock_num);
  if (info != nullptr) {
    return info->Peek();
  } else {
    return -1;
  }
}

ssize_t HostNetwork::Recv(uint8_t sock_num, uint8_t *buf, size_t len) {
  auto *info = impl_->GetHostSocketInfo(sock_num);
  if (info != nullptr) {
    return info->Recv(buf, len);
  } else {
    return -1;
  }
}

////////////////////////////////////////////////////////////////////////////////
// Methods for checking the interpretation of the status value.

bool HostNetwork::StatusIsOpen(uint8_t status) {
  return status == HostSocketInfo::kStatusEstablished ||
         status == HostSocketInfo::kStatusCloseWait;
}

bool HostNetwork::StatusIsHalfClosed(uint8_t status) {
  return status == HostSocketInfo::kStatusCloseWait;
}

bool HostNetwork::StatusIsClosing(uint8_t status) {
  // We don't have a way to track the state of a TCP connection after we've
  // disconnected or closed the connection.
  return false;
}

////////////////////////////////////////////////////////////////////////////////

namespace {
void LogError(const int errnum, std::string_view message_prefix) {
  char message[128];
  if (!strerror_r(errnum, message, sizeof message)) {
    LOG(WARNING) << message_prefix << "; errno=" << errnum
                 << "; message=" << message;
  } else {
    LOG(WARNING) << message_prefix << "; errno=" << errnum;
  }
}

bool CloseSocket(const int socket_fd, std::string_view caller_name) {
  if (close(socket_fd) < 0) {
    const int errnum = errno;
    LogError(errnum, absl::StrCat(caller_name, ": failed to close socket"));
    return false;
  }
  LOG(INFO) << caller_name << ": closed socket " << socket_fd;
  return true;
}

int FindFreeInetPort(int socket_type, std::string_view caller_name) {
  int socket_fd = socket(AF_INET, socket_type, 0);
  if (socket_fd < 0) {
    const int errnum = errno;
    LogError(errnum, absl::StrCat(caller_name, ": failed to create a socket"));
    return -1;
  }
  LOG(INFO) << caller_name << ": opened socket " << socket_fd;

  struct sockaddr_in serv_addr;
  bzero((char *)&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = 0;
  if (bind(socket_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    const int errnum = errno;
    LogError(errnum, absl::StrCat(caller_name, ": failed to bind socket"));
    CloseSocket(socket_fd, caller_name);
    return -1;
  }

  socklen_t len = sizeof(serv_addr);
  if (getsockname(socket_fd, (struct sockaddr *)&serv_addr, &len) == -1) {
    const int errnum = errno;
    LogError(errnum, absl::StrCat(caller_name, ": getsockname failed"));
    CloseSocket(socket_fd, caller_name);
    return -1;
  }

  const int tcp_port = ntohs(serv_addr.sin_port);
  LOG(INFO) << caller_name << ": port " << tcp_port << " is free";

  if (!CloseSocket(socket_fd, caller_name)) {
    // Port is still in use by the socket, so we can't claim it is free.
    LOG(ERROR) << caller_name << ": socket still holds port " << tcp_port;
    return -1;
  }

  return tcp_port;
}
}  // namespace

// static
int HostNetwork::FindFreeTcpPort() {
  return FindFreeInetPort(SOCK_STREAM, "FindFreeTcpPort");
}

}  // namespace mcunet_host
