#include "platform_network.h"

#include <McuCore.h>

namespace mcunet {

#if MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
namespace {
PlatformNetworkInterface* g_platform_network = nullptr;
}  // namespace

void PlatformNetwork::SetPlatformNetworkImplementation(
    PlatformNetworkInterface* platform_network) {
  g_platform_network = platform_network;
}

#define CALL_PNAPI_METHOD(NAME, ARGS)                               \
  MCU_CHECK_NE(g_platform_network, nullptr) << MCU_FLASHSTR(#NAME); \
  return g_platform_network->NAME ARGS;

#endif  // MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION

uint8_t PlatformNetwork::SocketStatus(uint8_t sock_num) {
#if MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
  CALL_PNAPI_METHOD(SocketStatus, (sock_num));
#else
  EthernetClient client(sock_num);
  return client.status();
#endif
}

int PlatformNetwork::FindUnusedSocket() {
#if MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
  CALL_PNAPI_METHOD(FindUnusedSocket, ());
#else
  for (int sock_num = 0; sock_num < MAX_SOCK_NUM; ++sock_num) {
    if (SocketIsClosed(sock_num)) {
      return sock_num;
    }
  }
  return -1;
#endif
}

bool PlatformNetwork::InitializeTcpListenerSocket(uint8_t sock_num,
                                                  uint16_t tcp_port) {
#if MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
  CALL_PNAPI_METHOD(InitializeTcpListenerSocket, (sock_num, tcp_port));
#else   // !MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
  MCU_DCHECK_LT(sock_num, MAX_SOCK_NUM);
  EthernetClient client(sock_num);
  const auto status = client.status();
  // EthernetClass::_server_port is the port that a socket is listening to.
  MCU_VLOG(3) << MCU_FLASHSTR("PlatformNetwork::InitializeTcpListenerSocket(")
              << sock_num << MCU_FLASHSTR(", ") << tcp_port
              << MCU_FLASHSTR(") _server_port is ")
              << EthernetClass::_server_port[sock_num]
              << MCU_FLASHSTR(", status=") << status;
  if (status == SnSR::CLOSED) {
    if (EthernetClass::_server_port[sock_num] == tcp_port ||
        EthernetClass::_server_port[sock_num] == 0) {
      // TODO(jamessynge): Improve the underlying impl so that errors are
      // actually returned from socket and listen.
      ::socket(sock_num, SnMR::TCP, tcp_port, 0);
      ::listen(sock_num);
      EthernetClass::_server_port[sock_num] = tcp_port;
      return true;
    } else {
      MCU_VLOG(1) << MCU_FLASHSTR("Socket ") << sock_num
                  << MCU_FLASHSTR(" already used for port ")
                  << EthernetClass::_server_port[sock_num]
                  << MCU_FLASHSTR(", not ") << tcp_port;
    }
  } else {
    MCU_VLOG(1) << MCU_FLASHSTR("Socket ") << sock_num
                << MCU_FLASHSTR(" not closed; status=") << status;
  }
  return false;
#endif  // MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
}

bool PlatformNetwork::SocketIsInTcpConnectionLifecycle(uint8_t sock_num) {
  MCU_DCHECK_LT(sock_num, MAX_SOCK_NUM);
#if MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
  CALL_PNAPI_METHOD(SocketIsInTcpConnectionLifecycle, (sock_num));
#else   // !MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
  switch (SocketStatus(sock_num)) {
    case SnSR::SYNRECV:
    case SnSR::ESTABLISHED:
    case SnSR::CLOSE_WAIT:
    case SnSR::FIN_WAIT:
    case SnSR::CLOSING:
    case SnSR::TIME_WAIT:
    case SnSR::LAST_ACK:
    case SnSR::INIT:
    case SnSR::SYNSENT:
      return true;
  }
  return false;
#endif  // MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
}

bool PlatformNetwork::SocketIsTcpListener(uint8_t sock_num, uint16_t tcp_port) {
  MCU_DCHECK_LT(sock_num, MAX_SOCK_NUM);
#if MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
  CALL_PNAPI_METHOD(SocketIsTcpListener, (sock_num, tcp_port));
#else   // !MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
  return EthernetClass::_server_port[sock_num] == tcp_port &&
         SocketStatus(sock_num) == SnSR::LISTEN;
#endif  // MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
}

// We don't have a host AND embedded implementation of ::disconnect, so we can't
// support MCU_PLATFORM_NETWORK_IS_OPTIONAL here.
bool PlatformNetwork::DisconnectSocket(uint8_t sock_num) {
  MCU_DCHECK_LT(sock_num, MAX_SOCK_NUM);
#if MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
  CALL_PNAPI_METHOD(DisconnectSocket, (sock_num));
#else   // !MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
  ::disconnect(sock_num);
  return true;
#endif  // MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
}

// We don't have a host AND embedded implementation of ::close, so we can't
// support MCU_PLATFORM_NETWORK_IS_OPTIONAL here.
bool PlatformNetwork::CloseSocket(uint8_t sock_num) {
  MCU_DCHECK_LT(sock_num, MAX_SOCK_NUM);
#if MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
  CALL_PNAPI_METHOD(CloseSocket, (sock_num));
#else   // !MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
  ::close(sock_num);
  return true;
#endif  // MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
}

bool PlatformNetwork::SocketIsClosed(uint8_t sock_num) {
  MCU_DCHECK_LT(sock_num, MAX_SOCK_NUM);
#if MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
  CALL_PNAPI_METHOD(SocketIsClosed, (sock_num));
#else   // !MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
  EthernetClient client(sock_num);
  return client.status() == SnSR::CLOSED;
#endif  // MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
}

bool PlatformNetwork::StatusIsOpen(uint8_t status) {
#if MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
  CALL_PNAPI_METHOD(StatusIsOpen, (status));
#else   // !MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
  return status == SnSR::ESTABLISHED || status == SnSR::CLOSE_WAIT;
#endif  // MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
}

bool PlatformNetwork::StatusIsHalfOpen(uint8_t status) {
#if MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
  CALL_PNAPI_METHOD(StatusIsHalfOpen, (status));
#else   // !MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
  return status == SnSR::CLOSE_WAIT;
#endif  // MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
}

bool PlatformNetwork::StatusIsClosing(uint8_t status) {
#if MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
  CALL_PNAPI_METHOD(StatusIsClosing, (status));
#else   // !MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
  switch (status) {
    case SnSR::FIN_WAIT:
    case SnSR::CLOSING:
    case SnSR::TIME_WAIT:
    case SnSR::LAST_ACK:
      return true;
  }
  return false;
#endif  // MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
}

}  // namespace mcunet
