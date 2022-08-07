#include "addresses.h"

#include <McuCore.h>

#include "eeprom_tags.h"

namespace mcunet {
namespace {
// Helper for calls to WriteEntryToCursor.
mcucore::Status WriteAddressesToRegion(mcucore::EepromRegion& region,
                                       const Addresses& addresses) {
  return addresses.WriteToRegion(region);
}
}  // namespace

mcucore::Status Addresses::ReadEepromEntry(mcucore::EepromTlv& eeprom_tlv,
                                           const OuiPrefix* oui_prefix) {
  MCU_VLOG(4) << MCU_PSD("Addresses::ReadEepromEntry");
  MCU_ASSIGN_OR_RETURN(auto region, eeprom_tlv.FindEntry(GetAddressesTag()));
  MCU_RETURN_IF_ERROR(ReadFromRegion(region));
  if (oui_prefix && !ethernet.HasOuiPrefix(*oui_prefix)) {
    auto status =
        mcucore::FailedPreconditionError(MCU_PSV("Stored OUI prefix mismatch"));
    MCU_VLOG(2) << status.message();
    return status;
  }
  return mcucore::OkStatus();
}

mcucore::Status Addresses::WriteEepromEntry(
    mcucore::EepromTlv& eeprom_tlv) const {
  MCU_VLOG(4) << MCU_PSD("Addresses::WriteEepromEntry");
  return eeprom_tlv.WriteEntryToCursor(GetAddressesTag(), 4 + 6,
                                       WriteAddressesToRegion, *this);
}

void Addresses::GenerateAddresses(const OuiPrefix* const oui_prefix) {
  ethernet.GenerateAddress(oui_prefix);
  ip.GenerateAddress();
}

void Addresses::InsertInto(mcucore::OPrintStream& strm) const {
  strm << MCU_NAME_VAL(ethernet) << MCU_NAME_VAL(ip);
}

mcucore::Status Addresses::ReadFromRegion(mcucore::EepromRegionReader& region) {
  MCU_RETURN_IF_ERROR(ethernet.ReadFromRegion(region));
  MCU_RETURN_IF_ERROR(ip.ReadFromRegion(region));
  return mcucore::OkStatus();
}

mcucore::Status Addresses::WriteToRegion(mcucore::EepromRegion& region) const {
  MCU_RETURN_IF_ERROR(ethernet.WriteToRegion(region));
  MCU_RETURN_IF_ERROR(ip.WriteToRegion(region));
  return mcucore::OkStatus();
}

bool operator==(const Addresses& lhs, const Addresses& rhs) {
  return lhs.ethernet == rhs.ethernet && lhs.ip == rhs.ip;
}

bool operator<(const Addresses& lhs, const Addresses& rhs) {
  return lhs.ethernet < rhs.ethernet ||
         (lhs.ethernet == rhs.ethernet && lhs.ip < rhs.ip);
}

}  // namespace mcunet
