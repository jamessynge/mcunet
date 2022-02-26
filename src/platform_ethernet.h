#ifndef MCUNET_SRC_PLATFORM_ETHERNET_H_
#define MCUNET_SRC_PLATFORM_ETHERNET_H_

// Exports the Ethernet support needed by some parts of Tiny Alpaca Server.
//
// TODO(jamessynge): Remove support for operating on half-closed sockets (i.e.
// treat that as a request by the client to close the socket, not as the end of
// the request data).
//
// Author: james.synge@gmail.com

#include <McuCore.h>  // IWYU pragma: export

#ifdef ARDUINO

#include <Client.h>          // IWYU pragma: export
#include <Ethernet5500.h>    // IWYU pragma: export
#include <IPAddress.h>       // IWYU pragma: export
#include <Stream.h>          // IWYU pragma: export
#include <utility/socket.h>  // IWYU pragma: export

#elif MCU_EMBEDDED_TARGET

#error "No support known for this platform."

#elif MCU_HOST_TARGET

#include "extras/host/arduino/client.h"             // IWYU pragma: export
#include "extras/host/arduino/ip_address.h"         // IWYU pragma: export
#include "extras/host/ethernet5500/ethernet5500.h"  // IWYU pragma: export
#include "extras/host/ethernet5500/host_sockets.h"  // IWYU pragma: export

#endif  // ARDUINO

namespace mcunet {

#ifndef MCU_HAS_PLATFORM_ETHERNET_INTERFACE
#if MCU_HOST_TARGET
#define MCU_HAS_PLATFORM_ETHERNET_INTERFACE 1
#endif  // MCU_HOST_TARGET
#endif  // MCU_HAS_PLATFORM_ETHERNET_INTERFACE

#if MCU_HAS_PLATFORM_ETHERNET_INTERFACE
class PlatformEthernetInterface {
 public:
  virtual ~PlatformEthernetInterface();

  // Returns the implementation defined status value for the specified socket.
  virtual uint8_t SocketStatus(uint8_t sock_num) = 0;

  // Finds a hardware socket that is closed, and returns its socket number.
  // Returns -1 if there is no such socket.
  virtual int FindUnusedSocket() = 0;

  // Set socket 'sock_num' to listen for new TCP connections on port 'tcp_port',
  // regardless of what that socket is doing now. Returns true if able to do so;
  // false if not (e.g. if sock_num or tcp_port is invalid).
  virtual bool InitializeTcpListenerSocket(uint8_t sock_num,
                                           uint16_t tcp_port) = 0;

  // Returns true if the hardware socket is being used for TCP and is not
  // LISTENING; if so, then it is best not to repurpose the hardware socket.
  virtual bool SocketIsInTcpConnectionLifecycle(uint8_t sock_num) = 0;

  // Returns true if the hardware socket is listening for TCP connections.
  virtual bool SocketIsTcpListener(uint8_t sock_num, uint16_t tcp_port) = 0;

  // Initiates a DISCONNECT of a TCP socket.
  virtual bool DisconnectSocket(uint8_t sock_num) = 0;

  // Forces a socket to be closed, with no packets sent out.
  virtual bool CloseSocket(uint8_t sock_num) = 0;

  // Returns true if the status indicates that the TCP connection is at least
  // half-open.
  virtual bool StatusIsOpen(uint8_t status) = 0;

  // Returns true if the status indicates that the TCP connection is half-open.
  virtual bool StatusIsHalfOpen(uint8_t status) = 0;

  // Returns true if the status indicates that the TCP connection is in the
  // process of closing (e.g. FIN_WAIT).
  virtual bool StatusIsClosing(uint8_t status) = 0;
};
#endif  // MCU_HAS_PLATFORM_ETHERNET_INTERFACE

// Helper for testing with the same API on host and embedded.
struct PlatformEthernet {
#if MCU_HAS_PLATFORM_ETHERNET_INTERFACE
  static void SetPlatformEthernetImplementation(
      PlatformEthernetInterface* platform_ethernet_impl);
#endif  // MCU_HAS_PLATFORM_ETHERNET_INTERFACE

  // Returns the implementation defined status value for the specified socket.
  // See SnSR in w5500.h.
  static uint8_t SocketStatus(uint8_t sock_num);

  // Finds a hardware socket that is closed, and returns its socket number.
  // Returns -1 if there is no such socket.
  static int FindUnusedSocket();

  // Set socket 'sock_num' to listen for new TCP connections on port 'tcp_port',
  // regardless of what that socket is doing now. Returns true if able to do so;
  // false if not (e.g. if sock_num or tcp_port is invalid).
  static bool InitializeTcpListenerSocket(uint8_t sock_num, uint16_t tcp_port);

  // Returns true if the hardware socket is being used for TCP and is not
  // LISTENING; if so, then it is best not to repurpose the hardware socket.
  static bool SocketIsInTcpConnectionLifecycle(uint8_t sock_num);

  // Returns true if the hardware socket is listening for TCP connections.
  static bool SocketIsTcpListener(uint8_t sock_num, uint16_t tcp_port);

  // Initiates a DISCONNECT of a TCP socket.
  static bool DisconnectSocket(uint8_t sock_num);

  // Forces a socket to be closed, with no packets sent out.
  static bool CloseSocket(uint8_t sock_num);

  // Returns true if the status indicates that the TCP connection is at least
  // half-open.
  static bool StatusIsOpen(uint8_t status);

  // Returns true if the status indicates that the TCP connection is half-open.
  static bool StatusIsHalfOpen(uint8_t status);

  // Returns true if the status indicates that the TCP connection is in the
  // process of closing (e.g. FIN_WAIT).
  static bool StatusIsClosing(uint8_t status);
};

}  // namespace mcunet

#endif  // MCUNET_SRC_PLATFORM_ETHERNET_H_
