#ifndef MCUNET_EXTRAS_HOST_ETHERNET5500_ETHERNET_CLASS_H_
#define MCUNET_EXTRAS_HOST_ETHERNET5500_ETHERNET_CLASS_H_

// Just enough of EthernetClass for Tiny Alpaca Server to compile on host, maybe
// to be a TCP server.

#include "extras/host/arduino/ip_address.h"  // IWYU pragma: export
#include "extras/host/ethernet5500/dhcp_class.h"
#include "extras/host/ethernet5500/ethernet_config.h"

#define DHCP_CHECK_NONE (0)
#define DHCP_CHECK_RENEW_FAIL (1)
#define DHCP_CHECK_RENEW_OK (2)
#define DHCP_CHECK_REBIND_FAIL (3)
#define DHCP_CHECK_REBIND_OK (4)

class EthernetClass {
 public:
  EthernetClass();

  // init(maxSockNum) initializes the W5500 with the specified number of
  // hardware sockets enabled, where that number is 1, 2, 4 or 8. A W5500 has
  // 32KB of buffer for receive and transmit buffers, divided among the sockets
  // as follows based on the number of hardware sockets:
  //
  // maxSockNum = 1 Socket 0 -> RX/TX Buffer 16KB
  // maxSockNum = 2 Socket 0, 1 -> RX/TX Buffer 8KB
  // maxSockNum = 4 Socket 0...3 -> RX/TX Buffer 4KB
  // maxSockNum = 8 (Standard) all sockets -> RX/TX Buffer 2KB
  //
  // Note: MAX_SOCK_NUM is set at compile type, and doesn't reflect the value
  // passed to init().
  void init(uint8_t maxSockNum = 8) {}

  // You need to call setCsPin first.
  uint8_t softreset() { return 1; }

  // You need to call setRstPin first.
  void hardreset() {}

  void setRstPin(uint8_t pinRST) {}
  void setCsPin(uint8_t pinCS) {}
  void setDhcp(DhcpClass* dhcp) { _dhcp = dhcp; }

  // Declaring functions in the order called.
  void setHostname(const char* hostname);

  // Pretend version of begin, does nothing.
  // On embedded device, sets up the network chip, starts it running.
  template <class... T>
  static int begin(T&&...) {
    return 1;
  }

  // Maintains lease on DHCP, returns DHCP_CHECK_RENEW_OK, etc.
  int maintain() { return DHCP_CHECK_RENEW_OK; }

  // returns the linkstate, 1 = linked, 0 = no link
  uint8_t link() const { return 1; }

  void macAddress(uint8_t mac[]) {
    for (int ndx = 0; ndx < 6; ++ndx) {
      auto nibble2 = ndx * 2 + 1;
      auto nibble1 = ndx * 2;
      mac[ndx] = (nibble2 << 4) + nibble1;
    }
  }

  IPAddress localIP() { return IPAddress(127, 0, 9, 9); }
  IPAddress subnetMask() { return IPAddress(255, 255, 0, 0); }
  IPAddress gatewayIP() { return IPAddress(127, 0, 0, 1); }
  IPAddress dnsServerIP() { return IPAddress(127, 0, 8, 8); }

  // Exposed fields.
  uint8_t _maxSockNum;  // NOLINT
  uint8_t _pinCS;       // NOLINT
  uint8_t _pinRST;      // NOLINT

  // Exposed static fields. Should really be members of a separate struct.
  static uint8_t _state[MAX_SOCK_NUM];  // NOLINT
  // Records the port that is/was last being listened to on the socket.
  static uint16_t _server_port[MAX_SOCK_NUM];  // NOLINT

 private:
  IPAddress _dnsServerAddress;  // NOLINT
  DhcpClass* _dhcp;             // NOLINT
  char _customHostname[32];     // NOLINT
};

extern EthernetClass Ethernet;

#endif  // MCUNET_EXTRAS_HOST_ETHERNET5500_ETHERNET_CLASS_H_
