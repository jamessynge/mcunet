#include "eeprom_tags.h"

#include <McuCore.h>

// REMEMBER: Don't change the domain or id numbers, else the tagged EEPROM
// entries will be orphaned.
MCU_DEFINE_NAMED_DOMAIN(McuNetDomain, 100);

namespace mcunet {

mcucore::EepromTag GetAddressesTag() {
  // REMEMBER: Don't change the values here, or the tagged EEPROM entries will
  // be orphaned.
  return {.domain = MCU_DOMAIN(McuNetDomain), .id = 1};
}

}  // namespace mcunet
