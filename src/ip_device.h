#ifndef MCUNET_SRC_IP_DEVICE_H_
#define MCUNET_SRC_IP_DEVICE_H_

// IpDevice takes care of setting up the Internet Offload device of the embedded
// device; currently supporting the WIZnet W5500 of a RobotDyn Mega ETH board.
//
// Author: james.synge@gmail.com

#include <McuCore.h>

#include "ethernet_address.h"
#include "platform_network.h"

namespace mcunet {

struct Mega2560Eth {
  // Configures the pins necessary for talking to the W5500 on the RobotDyn Mega
  // 2560 ETH board. max_sock_num can be 8, 4, 2, or 1. An Alpaca server needs
  // at least two UDP ports (DHCP and Alpaca Discovery), plus at least 1 TCP
  // port for HTTP.
  static void SetupW5500(uint8_t max_sock_num = MAX_SOCK_NUM);
};

class IpDevice {
 public:
  // Set the MAC address of the Ethernet chip and get a DHCP assigned IP address
  // or use a "randomly" generated address. Returns false if unable to configure
  // addresses or if there is no Ethernet hardware, else returns true.
  //
  // If it is necessary to generate an Ethernet (MAC) address, the Arduino
  // random number library is used, so be sure to seed it according to the level
  // of randomness you want in the generated address; if you don't set the seed,
  // the same sequence of numbers is always produced.
  //
  // It *MAY* help with identifying devices on the network that are using this
  // software if they have a known "Organizationally Unique Identifier" (the
  // first 3 bytes of the MAC address). Therefore, setup() takes an optional
  // OuiPrefix allowing the caller to provide such a prefix.
  //
  // Call Mega2560Eth::SetupW5500 prior to calling this method.
  mcucore::Status InitializeNetworking(mcucore::EepromTlv& eeprom_tlv,
                                       const OuiPrefix* oui_prefix = nullptr);

  // Ensures that the DHCP lease (if there is one) is maintained. Returns a
  // DHCP_CHECK_* value; definitions in Ethernet5500's Dhcp.h.
  int MaintainDhcpLease();

  static void PrintNetworkAddresses();

 private:
  bool using_dhcp_{false};
};

}  // namespace mcunet

#endif  // MCUNET_SRC_IP_DEVICE_H_
