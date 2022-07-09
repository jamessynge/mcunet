#include "platform_network.h"

#include <McuCore.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

namespace mcunet {

#if MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
#define CALL_PNAPI_METHOD(NAME, ARGS) \
  return PlatformNetworkInterface::GetImplementationOrDie()->NAME ARGS;

#endif  // MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION

////////////////////////////////////////////////////////////////////////////////
// Methods getting the status of a socket.

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

uint16_t PlatformNetwork::SocketIsTcpListener(uint8_t sock_num) {
  MCU_DCHECK_LT(sock_num, MAX_SOCK_NUM);
#if MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
  CALL_PNAPI_METHOD(SocketIsTcpListener, (sock_num));
#else   // !MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
  if (SocketStatus(sock_num) == SnSR::LISTEN) {
    return EthernetClass::_server_port[sock_num];
  }
  return 0;
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

bool PlatformNetwork::SocketIsHalfClosed(uint8_t sock_num) {
  MCU_DCHECK_LT(sock_num, MAX_SOCK_NUM);
#if MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
  CALL_PNAPI_METHOD(SocketIsHalfClosed, (sock_num));
#else   // !MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
  return SocketStatus(sock_num) == SnSR::CLOSE_WAIT;
#endif  // MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
}

bool PlatformNetwork::SocketIsClosed(uint8_t sock_num) {
  MCU_DCHECK_LT(sock_num, MAX_SOCK_NUM);
#if MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
  CALL_PNAPI_METHOD(SocketIsClosed, (sock_num));
#else   // !MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
  return SocketStatus(sock_num) == SnSR::CLOSED;
#endif  // MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
}

uint8_t PlatformNetwork::SocketStatus(uint8_t sock_num) {
#if MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
  CALL_PNAPI_METHOD(SocketStatus, (sock_num));
#else
  EthernetClient client(sock_num);
  return client.status();
#endif
}

////////////////////////////////////////////////////////////////////////////////
// Methods modifying sockets.

bool PlatformNetwork::InitializeTcpListenerSocket(uint8_t sock_num,
                                                  uint16_t tcp_port) {
#if MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
  CALL_PNAPI_METHOD(InitializeTcpListenerSocket, (sock_num, tcp_port));
#else   // !MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
  MCU_DCHECK_LT(sock_num, MAX_SOCK_NUM);
  const auto status = SocketStatus(sock_num);
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

bool PlatformNetwork::AcceptConnection(uint8_t sock_num) {
  MCU_DCHECK_LT(sock_num, MAX_SOCK_NUM);
#if MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
  CALL_PNAPI_METHOD(AcceptConnection, (sock_num));
#else   // !MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
  // There really isn't any need to call this method when using a W5500.
  MCU_VLOG(3) << MCU_FLASHSTR(
      "PlatformNetwork::AcceptConnection called unexpectedly");
  return false;
#endif  // MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
}

bool PlatformNetwork::DisconnectSocket(uint8_t sock_num) {
  MCU_DCHECK_LT(sock_num, MAX_SOCK_NUM);
#if MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
  CALL_PNAPI_METHOD(DisconnectSocket, (sock_num));
#else   // !MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
  ::disconnect(sock_num);
  return true;
#endif  // MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
}

bool PlatformNetwork::CloseSocket(uint8_t sock_num) {
  MCU_DCHECK_LT(sock_num, MAX_SOCK_NUM);
#if MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
  CALL_PNAPI_METHOD(CloseSocket, (sock_num));
#else   // !MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
  ::close(sock_num);
  return true;
#endif  // MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
}

////////////////////////////////////////////////////////////////////////////////
// Methods using open sockets.

ssize_t PlatformNetwork::Send(uint8_t sock_num, const uint8_t* buf,
                              size_t len) {
  MCU_DCHECK_LT(sock_num, MAX_SOCK_NUM);
#if MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
  CALL_PNAPI_METHOD(Send, (sock_num, buf, len));
#else   // !MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
  return ::send(sock_num, buf, len);
#endif  // MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
}

void PlatformNetwork::Flush(uint8_t sock_num) {
  MCU_DCHECK_LT(sock_num, MAX_SOCK_NUM);
#if MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
  CALL_PNAPI_METHOD(Flush, (sock_num));
#else   // !MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
  EthernetClient client(sock_num);
  return client.flush();
#endif  // MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
}

ssize_t PlatformNetwork::AvailableBytes(uint8_t sock_num) {
  MCU_DCHECK_LT(sock_num, MAX_SOCK_NUM);
#if MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
  CALL_PNAPI_METHOD(AvailableBytes, (sock_num));
#else   // !MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
  EthernetClient client(sock_num);
  return client.available();
#endif  // MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
}

int PlatformNetwork::Peek(uint8_t sock_num) {
  MCU_DCHECK_LT(sock_num, MAX_SOCK_NUM);
#if MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
  CALL_PNAPI_METHOD(Peek, (sock_num));
#else   // !MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
  EthernetClient client(sock_num);
  return client.peek();
#endif  // MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
}

ssize_t PlatformNetwork::Recv(uint8_t sock_num, uint8_t* buf, size_t len) {
  MCU_DCHECK_LT(sock_num, MAX_SOCK_NUM);
#if MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
  CALL_PNAPI_METHOD(Recv, (sock_num, buf, len));
#else   // !MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
  return ::recv(sock_num, buf, len);
#endif  // MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
}

////////////////////////////////////////////////////////////////////////////////
// Methods for checking the interpretation of the
// status value.

bool PlatformNetwork::StatusIsOpen(uint8_t status) {
#if MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
  CALL_PNAPI_METHOD(StatusIsOpen, (status));
#else   // !MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
  return status == SnSR::ESTABLISHED || status == SnSR::CLOSE_WAIT;
#endif  // MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
}

bool PlatformNetwork::StatusIsHalfClosed(uint8_t status) {
#if MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
  CALL_PNAPI_METHOD(StatusIsHalfClosed, (status));
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
