#ifndef MCUNET_SRC_EEPROM_TAGS_H_
#define MCUNET_SRC_EEPROM_TAGS_H_

// Provides function(s) for returning the tag(s) used to identify EEPROM entries
// owned by McuNet.
//
// Author: james.synge@gmail.com

#include <McuCore.h>

namespace mcunet {

// Tag for use by mcunet::Addresses.
mcucore::EepromTag GetAddressesTag();

}  // namespace mcunet

#endif  // MCUNET_SRC_EEPROM_TAGS_H_
