#ifndef MCUNET_SRC_W5500_ENUM_NAMES_H_
#define MCUNET_SRC_W5500_ENUM_NAMES_H_

#include <McuCore.h>

namespace mcunet {

struct SnSRName {
  explicit SnSRName(uint8_t value) : value(value) {}
  SnSRName(const SnSRName&) = default;
  uint8_t value;
};
size_t PrintValueTo(SnSRName value, Print& out);

}  // namespace mcunet

#endif  // MCUNET_SRC_W5500_ENUM_NAMES_H_
