#ifndef MCUNET_SRC_IP_ADDRESS_H_
#define MCUNET_SRC_IP_ADDRESS_H_

// mcunet::IpAddress extends the IPAddress class provided by the Arduino core
// library to support saving an IPv4 address to EEPROM and later reading it back
// from EEPROM.
//
// A note on the name: I originally called this SaveableIPAddress, but really
// didn't like that name, so chose this name that is only different from the
// Arduino name in namespace and capitalization for... reasons. If really
// necessary, it could be called IpAddr, InetAddress, InetAddr or
// InternetAddress.

#include <McuCore.h>

#include "platform_ethernet.h"

namespace mcunet {

// Extends the IPAddress class provided by the Arduino core library to support
// saving an IPv4 address to EEPROM and later reading it back from EEPROM.
class IpAddress : public ::IPAddress {
 public:
  // Inherit the base class constructors.
  using ::IPAddress::IPAddress;

  // Generates a Link-Local IPv4 address.
  void GenerateAddress();

  // Read the address from the region, starting at the cursor.
  mcucore::Status ReadFromRegion(mcucore::EepromRegionReader& region);

  // Write the address to the region, starting at the cursor.
  mcucore::Status WriteToRegion(mcucore::EepromRegion& region) const;
};

// Not expecting to use these operators in an embedded environment, but it's
// convenient to define them here.
bool operator<(const IpAddress& lhs, const IpAddress& rhs);
inline bool operator!=(const IpAddress& lhs, const IpAddress& rhs) {
  return !(lhs == rhs);
}

}  // namespace mcunet

#endif  // MCUNET_SRC_IP_ADDRESS_H_
