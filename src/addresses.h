#ifndef MCUNET_SRC_ADDRESSES_H_
#define MCUNET_SRC_ADDRESSES_H_

// Support for generation and EEPROM persistence of a pair of addresses, one
// Ethernet and one Internet. This is useful because it allows us to generate
// random addresses when we first boot up a sketch, and then use those same
// addresses each time the sketch boots up in the future; this may make it
// easier for the person using the sketch to find their device on the LAN.
//
// Author: james.synge@gmail.com

#include <McuCore.h>

#include "ethernet_address.h"
#include "ip_address.h"

namespace mcunet {

struct Addresses {
  // Restore the addresses from an entry managed using EepromTlv. Returns OK if
  // a valid entry is found, else an error. If the stored Ethernet address is
  // not a unicast address, or if an OuiPrefix is provided and the stored
  // Ethernet address does not start with that prefix, then a FailedPrecondition
  // error is returned.
  mcucore::Status ReadEepromEntry(mcucore::EepromTlv& eeprom_tlv,
                                  const OuiPrefix* oui_prefix = nullptr);

  // Save in an EepromTlv entry. Returns OK if successful, else an error.
  mcucore::Status WriteEepromEntry(mcucore::EepromTlv& eeprom_tlv) const;

  // Randomly generate the addresses, using the Generate method of each address
  // class. The Ethernet address has the specified OuiPrefix if supplied. No
  // support is provided for detecting conflicts with other users of the
  // generated addresses. The Arduino random number library is used, so be sure
  // to seed it according to the level of randomness you want in the generated
  // address; if you don't set the seed, the same sequence of numbers is always
  // produced.
  void GenerateAddresses(const OuiPrefix* oui_prefix = nullptr);

  // Insert formatted addresses into the output stream, e.g. for logging.
  void InsertInto(mcucore::OPrintStream& strm) const;

  // Read the addresses from the region, starting at the cursor. This is exposed
  // for testing.
  mcucore::Status ReadFromRegion(mcucore::EepromRegionReader& region);

  // Write the addresses to the region, starting at the cursor. This is exposed
  // for testing.
  mcucore::Status WriteToRegion(mcucore::EepromRegion& region) const;

  EthernetAddress ethernet;
  IpAddress ip;
};

// May not use these operators in an embedded environment, but it's convenient
// to define them here, rather than just for tests.
bool operator==(const Addresses& lhs, const Addresses& rhs);
inline bool operator!=(const Addresses& lhs, const Addresses& rhs) {
  return !(lhs == rhs);
}
bool operator<(const Addresses& lhs, const Addresses& rhs);

}  // namespace mcunet

#endif  // MCUNET_SRC_ADDRESSES_H_
