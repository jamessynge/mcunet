#ifndef MCUNET_SRC_IP_DEVICE_H_
#define MCUNET_SRC_IP_DEVICE_H_

// IpDevice takes care of setting up the Internet Offload device of the embedded
// device; currently supporting the WIZnet W5500 of a RobotDyn Mega ETH board.
//
// Author: james.synge@gmail.com

#include <McuCore.h>

#include "addresses.h"
#include "platform_ethernet.h"

namespace mcunet {

struct Mega2560Eth {
  // Configures the pins necessary for talking to the W5500 on the RobotDyn Mega
  // 2560 ETH board. max_sock_num can be 8, 4, 2, or 1. An Alpaca server needs
  // at least two UDP ports (DHCP and Alpaca Discovery), plus at least 1 TCP
  // port for HTTP. Returns true if able to reset the W5500, otherwise false.
  static void SetW5500Configuration(uint8_t max_sock_num = MAX_SOCK_NUM);

  // Reset the W5500. Returns true if the W5500 has been reset (i.e. we can read
  // the register indicating that it is reset).
  static bool ResetW5500();
};

class IpDevice {
 public:
  // Order matters here; see IsLinked.
  enum EStatus : uint8_t {
    // The Ethernet hardware doesn't seem to be present.
    kNoHardware = 0,
    // The Ethernet connection doesn't seem to be working.
    kNoLink = 1,
    // Link is working, DHCP didn't assign an address.
    kLinkUpDhcpFailed = 2,
    // Assigned an address via DHCP, which implies link is working.
    kLinkUpDhcpSuccessful = 3,
  };

  // Returns true if initialized with a working Ethernet link. This should
  // transition from true to false if the network cable is unplugged from a
  // working network.
  bool IsLinked() const { return status_ <= kNoLink; }

  // Uses Mega2560Eth to configure the Ethernet library and to reset the device,
  // sets the MAC address of the Ethernet chip and get a DHCP assigned IP
  // address or use a generated address. Returns an EStatus indicating the
  // result of the effort.
  //
  // This may be called multiple times, but each time it will reset the
  // hardware, so we generally prefer to do so only if
  //
  // It *MAY* help with identifying devices on the network that are using this
  // software if they have a known "Organizationally Unique Identifier" (the
  // first 3 bytes of the MAC address). Therefore, setup() takes an optional
  // OuiPrefix allowing the caller to provide such a prefix.
  EStatus StartNetworking(uint8_t max_sock_num = MAX_SOCK_NUM,
                          const OuiPrefix* oui_prefix = nullptr);

  // Ensures that the DHCP lease (if there is one) is maintained. Returns true
  // if there was no problem; otherwise sets status_ to kNoHardware and returns
  // false.
  bool MaintainDhcpLease();

  static void PrintNetworkAddresses();

 private:
  EStatus InitializeNetworking(const OuiPrefix* oui_prefix);

  EStatus status_{kNoHardware};
};

}  // namespace mcunet

#endif  // MCUNET_SRC_IP_DEVICE_H_
