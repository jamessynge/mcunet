#include "platform_ethernet.h"

#include <McuCore.h>

namespace mcunet {
#if MCU_HAS_PLATFORM_ETHERNET_INTERFACE
namespace {
PlatformEthernetInterface* g_platform_ethernet_impl = nullptr;
}  // namespace

void PlatformEthernet::SetPlatformEthernetImplementation(
    PlatformEthernetInterface* platform_ethernet_impl) {
  if (g_platform_ethernet_impl != nullptr &&
      platform_ethernet_impl != nullptr) {
    MCU_CHECK_EQ(g_platform_ethernet_impl, platform_ethernet_impl);
  }
  g_platform_ethernet_impl = platform_ethernet_impl;
}
#endif  // MCU_HAS_PLATFORM_ETHERNET_INTERFACE

uint8_t PlatformEthernet::SocketStatus(uint8_t sock_num) {
#if MCU_HAS_PLATFORM_ETHERNET_INTERFACE
#if !MCU_PLATFORM_ETHERNET_IS_OPTIONAL
  MCU_CHECK_NE(g_platform_ethernet_impl, nullptr);
#endif
  if (g_platform_ethernet_impl != nullptr) {
    return g_platform_ethernet_impl->SocketStatus(sock_num);
  }
#endif
  EthernetClient client(sock_num);
  return client.status();
}

int PlatformEthernet::FindUnusedSocket() {
#if MCU_HAS_PLATFORM_ETHERNET_INTERFACE
#if !MCU_PLATFORM_ETHERNET_IS_OPTIONAL
  MCU_CHECK_NE(g_platform_ethernet_impl, nullptr);
#endif
  if (g_platform_ethernet_impl != nullptr) {
    return g_platform_ethernet_impl->FindUnusedSocket();
  }
#endif

  for (int sock_num = 0; sock_num < MAX_SOCK_NUM; ++sock_num) {
    if (SocketIsClosed(sock_num)) {
      return sock_num;
    }
  }
  return -1;
}

// We don't have a host AND embedded implementation of ::socket and ::listen, so
// we can't support MCU_PLATFORM_ETHERNET_IS_OPTIONAL here.
bool PlatformEthernet::InitializeTcpListenerSocket(uint8_t sock_num,
                                                   uint16_t tcp_port) {
  MCU_VLOG(3) << MCU_FLASHSTR("PlatformEthernet::InitializeTcpListenerSocket(")
              << sock_num << MCU_FLASHSTR(", ") << tcp_port
              << MCU_FLASHSTR(") _server_port is ")
              << EthernetClass::_server_port[sock_num];
  MCU_DCHECK_LT(sock_num, MAX_SOCK_NUM);

#if MCU_HAS_PLATFORM_ETHERNET_INTERFACE
#if MCU_PLATFORM_ETHERNET_IS_OPTIONAL
#pragma message(                                   \
    "InitializeTcpListenerSocket doesn't support " \
    "MCU_PLATFORM_ETHERNET_IS_OPTIONAL")
#endif

  MCU_CHECK_NE(g_platform_ethernet_impl, nullptr);
  return g_platform_ethernet_impl->InitializeTcpListenerSocket(sock_num,
                                                               tcp_port);

#else  // !MCU_HAS_PLATFORM_ETHERNET_INTERFACE

  EthernetClient client(sock_num);
  MCU_VLOG(4) << MCU_FLASHSTR("Socket status: ") << client.status();
  if (client.status() == SnSR::CLOSED) {
    if (EthernetClass::_server_port[sock_num] == tcp_port ||
        EthernetClass::_server_port[sock_num] == 0) {
      // TODO(jamessynge): Improve the underlying impl so that errors are
      // actually returned from socket and listen.
      ::socket(sock_num, SnMR::TCP, tcp_port, 0);
      ::listen(sock_num);
      EthernetClass::_server_port[sock_num] = tcp_port;
      return true;
    }
  }
  return false;

#endif  // MCU_HAS_PLATFORM_ETHERNET_INTERFACE
}

bool PlatformEthernet::SocketIsInTcpConnectionLifecycle(uint8_t sock_num) {
  MCU_DCHECK_LT(sock_num, MAX_SOCK_NUM);
#if MCU_HAS_PLATFORM_ETHERNET_INTERFACE
#if !MCU_PLATFORM_ETHERNET_IS_OPTIONAL
  MCU_CHECK_NE(g_platform_ethernet_impl, nullptr);
#endif
  if (g_platform_ethernet_impl != nullptr) {
    return g_platform_ethernet_impl->SocketIsInTcpConnectionLifecycle(sock_num);
  }
#endif

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
}

bool PlatformEthernet::SocketIsTcpListener(uint8_t sock_num,
                                           uint16_t tcp_port) {
  MCU_DCHECK_LT(sock_num, MAX_SOCK_NUM);
#if MCU_HAS_PLATFORM_ETHERNET_INTERFACE
#if !MCU_PLATFORM_ETHERNET_IS_OPTIONAL
  MCU_CHECK_NE(g_platform_ethernet_impl, nullptr);
#endif
  if (g_platform_ethernet_impl != nullptr) {
    return g_platform_ethernet_impl->SocketIsTcpListener(sock_num, tcp_port);
  }
#endif

  return EthernetClass::_server_port[sock_num] == tcp_port &&
         SocketStatus(sock_num) == SnSR::LISTEN;
}

// We don't have a host AND embedded implementation of ::disconnect, so we can't
// support MCU_PLATFORM_ETHERNET_IS_OPTIONAL here.
bool PlatformEthernet::DisconnectSocket(uint8_t sock_num) {
  MCU_DCHECK_LT(sock_num, MAX_SOCK_NUM);
#if MCU_HAS_PLATFORM_ETHERNET_INTERFACE
#if MCU_PLATFORM_ETHERNET_IS_OPTIONAL
#pragma message( \
    "DisconnectSocket doesn't support MCU_PLATFORM_ETHERNET_IS_OPTIONAL")
#endif

  MCU_CHECK_NE(g_platform_ethernet_impl, nullptr);
  return g_platform_ethernet_impl->DisconnectSocket(sock_num);

#else  // !MCU_HAS_PLATFORM_ETHERNET_INTERFACE

  ::disconnect(sock_num);
  return true;

#endif  // MCU_HAS_PLATFORM_ETHERNET_INTERFACE
}

// We don't have a host AND embedded implementation of ::close, so we can't
// support MCU_PLATFORM_ETHERNET_IS_OPTIONAL here.
bool PlatformEthernet::CloseSocket(uint8_t sock_num) {
  MCU_DCHECK_LT(sock_num, MAX_SOCK_NUM);
#if MCU_HAS_PLATFORM_ETHERNET_INTERFACE
#if MCU_PLATFORM_ETHERNET_IS_OPTIONAL
#pragma message("CloseSocket doesn't support MCU_PLATFORM_ETHERNET_IS_OPTIONAL")
#endif

  MCU_CHECK_NE(g_platform_ethernet_impl, nullptr);
  return g_platform_ethernet_impl->CloseSocket(sock_num);

#else  // !MCU_HAS_PLATFORM_ETHERNET_INTERFACE

  ::close(sock_num);
  return true;

#endif  // MCU_HAS_PLATFORM_ETHERNET_INTERFACE
}

bool PlatformEthernet::SocketIsClosed(uint8_t sock_num) {
  MCU_DCHECK_LT(sock_num, MAX_SOCK_NUM);
#if MCU_HAS_PLATFORM_ETHERNET_INTERFACE
#if !MCU_PLATFORM_ETHERNET_IS_OPTIONAL
  MCU_CHECK_NE(g_platform_ethernet_impl, nullptr);
#endif

  if (g_platform_ethernet_impl != nullptr) {
    return g_platform_ethernet_impl->SocketIsClosed(sock_num);
  }

#endif

  EthernetClient client(sock_num);
  return client.status() == SnSR::CLOSED;
}

bool PlatformEthernet::StatusIsOpen(uint8_t status) {
#if MCU_HAS_PLATFORM_ETHERNET_INTERFACE
#if !MCU_PLATFORM_ETHERNET_IS_OPTIONAL
  MCU_CHECK_NE(g_platform_ethernet_impl, nullptr);
#endif

  if (g_platform_ethernet_impl != nullptr) {
    return g_platform_ethernet_impl->StatusIsOpen(status);
  }

#endif

  return status == SnSR::ESTABLISHED || status == SnSR::CLOSE_WAIT;
}

bool PlatformEthernet::StatusIsHalfOpen(uint8_t status) {
#if MCU_HAS_PLATFORM_ETHERNET_INTERFACE
#if !MCU_PLATFORM_ETHERNET_IS_OPTIONAL
  MCU_CHECK_NE(g_platform_ethernet_impl, nullptr);
#endif

  if (g_platform_ethernet_impl != nullptr) {
    return g_platform_ethernet_impl->StatusIsHalfOpen(status);
  }

#endif

  return status == SnSR::CLOSE_WAIT;
}

bool PlatformEthernet::StatusIsClosing(uint8_t status) {
#if MCU_HAS_PLATFORM_ETHERNET_INTERFACE
#if !MCU_PLATFORM_ETHERNET_IS_OPTIONAL
  MCU_CHECK_NE(g_platform_ethernet_impl, nullptr);
#endif

  if (g_platform_ethernet_impl != nullptr) {
    return g_platform_ethernet_impl->StatusIsClosing(status);
  }

#endif

  switch (status) {
    case SnSR::FIN_WAIT:
    case SnSR::CLOSING:
    case SnSR::TIME_WAIT:
    case SnSR::LAST_ACK:
      return true;
  }
  return false;
}

}  // namespace mcunet
