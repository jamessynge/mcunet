#include "ip_address.h"

// NOTE: I'm assuming here that IPAddress only supports IPv4 addresses. There
// are some Arduino libraries where IPAddress also supports IPv6 addresses, for
// which this code would need updating.

#include <McuCore.h>

namespace mcunet {

void IpAddress::GenerateAddress() {
  // A link-local address is in the range 169.254.1.0 to 169.254.254.255,
  // inclusive. Learn more: https://tools.ietf.org/html/rfc3927
  (*this)[0] = 169;
  (*this)[1] = 254;
  (*this)[2] = random(1, 255);
  (*this)[3] = random(0, 256);
  MCU_VLOG(5) << MCU_FLASHSTR("IpAddress::GenerateAddress: ") << *this;
}

mcucore::Status IpAddress::ReadFromRegion(mcucore::EepromRegionReader& region) {
  uint8_t bytes[4];
  if (!region.ReadBytes(bytes)) {
    return mcucore::UnknownError(MCU_PSV("Unable to read IP from EEPROM"));
  }
  (*this)[0] = bytes[0];
  (*this)[1] = bytes[1];
  (*this)[2] = bytes[2];
  (*this)[3] = bytes[3];
  return mcucore::OkStatus();
}

mcucore::Status IpAddress::WriteToRegion(mcucore::EepromRegion& region) const {
  bool result = region.Write((*this)[0]);
  result = result && region.Write((*this)[1]);
  result = result && region.Write((*this)[2]);
  result = result && region.Write((*this)[3]);
  if (result) {
    return mcucore::OkStatus();
  } else {
    return mcucore::UnknownError(MCU_PSV("Unable to write IP to EEPROM"));
  }
}

bool operator<(const IpAddress& lhs, const IpAddress& rhs) {
  for (int i = 0; i < 4; ++i) {
    if (lhs[i] < rhs[i]) {
      return true;
    } else if (lhs[i] > rhs[i]) {
      return false;
    }
  }
  return false;  // equal, not less
}

}  // namespace mcunet
