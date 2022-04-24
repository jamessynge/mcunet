#include "ethernet_address.h"

#include <McuCore.h>
#include <strings.h>

namespace mcunet {

OuiPrefix::OuiPrefix(uint8_t byte0, uint8_t byte1, uint8_t byte2) {
  bytes_[0] = ToOuiUnicast(byte0);
  bytes_[1] = byte1;
  bytes_[2] = byte2;
}

size_t OuiPrefix::printTo(Print& out) const {
  return mcucore::PrintWithEthernetFormatting(out, bytes_);
}

bool OuiPrefix::operator==(const OuiPrefix& other) const {
  // At what point is it better to use memcmp?
  return bytes_[0] == other.bytes_[0] && bytes_[1] == other.bytes_[1] &&
         bytes_[2] == other.bytes_[2];
}

uint8_t OuiPrefix::ToOuiUnicast(uint8_t first_byte) {
  // Make sure this is in the locally administered space (set the second from
  // lowest bit).
  first_byte |= 0b00000010;
  // And not a multicast address (clear the low-bit).
  first_byte &= 0b11111110;
  return first_byte;
}

////////////////////////////////////////////////////////////////////////////////

void EthernetAddress::GenerateAddress(const OuiPrefix* oui_prefix) {
  int first_index;
  if (oui_prefix) {
    first_index = 3;
    bytes[0] = (*oui_prefix)[0];
    bytes[1] = (*oui_prefix)[1];
    bytes[2] = (*oui_prefix)[2];
  } else {
    first_index = 0;
  }
  for (int i = first_index; i < 6; ++i) {
    bytes[i] = static_cast<uint8_t>(random(256));
  }
  bytes[0] = OuiPrefix::ToOuiUnicast(bytes[0]);
}

mcucore::Status EthernetAddress::ReadFromRegion(
    mcucore::EepromRegionReader& region) {
  if (region.ReadBytes(bytes)) {
    return mcucore::OkStatus();
  } else {
    return mcucore::UnknownError(MCU_PSV("Unable to read MAC from EEPROM"));
  }
}

mcucore::Status EthernetAddress::WriteToRegion(
    mcucore::EepromRegion& region) const {
  if (region.WriteBytes(bytes)) {
    return mcucore::OkStatus();
  } else {
    return mcucore::UnknownError(MCU_PSV("Unable to write MAC to EEPROM"));
  }
}

bool EthernetAddress::HasOuiPrefix(const OuiPrefix& oui_prefix) const {
  return (bytes[0] == oui_prefix[0] && bytes[1] == oui_prefix[1] &&
          bytes[2] == oui_prefix[2]);
}

size_t EthernetAddress::printTo(Print& out) const {
  return mcucore::PrintWithEthernetFormatting(out, bytes);
}

bool EthernetAddress::operator==(const EthernetAddress& other) const {
  return 0 == memcmp(bytes, other.bytes, sizeof bytes);
}

}  // namespace mcunet
