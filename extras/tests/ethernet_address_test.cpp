#include "ethernet_address.h"

#include <McuCore.h>

#include <cstring>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mcucore/extras/test_tools/print_value_to_std_string.h"
#include "mcucore/extras/test_tools/status_test_utils.h"

namespace mcunet {

bool operator<(const EthernetAddress& lhs, const EthernetAddress& rhs) {
  return std::memcmp(lhs.bytes, rhs.bytes, 6) < 0;
}

namespace test {
namespace {

using ::mcucore::PrintValueToStdString;
using ::mcucore::test::IsOk;
using ::testing::Not;

TEST(OutPrefixTest, NoBits) {
  OuiPrefix prefix(0, 0, 0);
  EXPECT_EQ(PrintValueToStdString(prefix), "02-00-00");
  EXPECT_EQ(prefix[0], 2);  // Set the second bit, so organizationally unique.
  EXPECT_EQ(prefix[1], 0);
  EXPECT_EQ(prefix[2], 0);
}

TEST(OutPrefixTest, AllBits) {
  OuiPrefix prefix = OuiPrefix(255, 255, 255);
  EXPECT_EQ(PrintValueToStdString(prefix), "FE-FF-FF");
  EXPECT_EQ(prefix[0], 254);  // Cleared the low-bit, so unicast.
  EXPECT_EQ(prefix[1], 255);
  EXPECT_EQ(prefix[2], 255);
}

TEST(EthernetAddressTest, GenerateAddress) {
  EthernetAddress address;
  address.GenerateAddress();
  EXPECT_EQ(address.bytes[0], OuiPrefix::ToOuiUnicast(address.bytes[0]));
}

TEST(EthernetAddressTest, GenerateAddressWithPrefix) {
  OuiPrefix prefix(0, 0x23, 0x45);
  EXPECT_EQ(prefix, OuiPrefix(0x02, 0x23, 0x45));
  EthernetAddress address{{2, 3, 4, 5, 6, 7}};
  EXPECT_FALSE(address.HasOuiPrefix(prefix));
  address.GenerateAddress(&prefix);
  EXPECT_TRUE(address.HasOuiPrefix(prefix));
  EXPECT_FALSE(address.HasOuiPrefix(OuiPrefix(0, 0, 0)));
}

TEST(EthernetAddressTest, GenerateUniqueAddresses) {
  std::set<EthernetAddress> addresses;
  for (int i = 0; i < 1000; ++i) {
    EthernetAddress address;
    address.GenerateAddress();
    EXPECT_EQ(addresses.find(address), addresses.end());
    addresses.insert(address);
  }
}

TEST(EthernetAddressTest, WriteAndRead) {
  EEPROMClass eeprom;
  {
    mcucore::EepromRegion region(eeprom, 0, eeprom.length());
    EXPECT_EQ(region.cursor(), 0);
    EthernetAddress address{{2, 3, 4, 5, 6, 7}};
    EXPECT_STATUS_OK(address.WriteToRegion(region));
    EXPECT_EQ(region.cursor(), 6);
    EXPECT_EQ(PrintValueToStdString(address), "02-03-04-05-06-07");
  }

  {
    mcucore::EepromRegionReader region(eeprom, 0, eeprom.length());
    EthernetAddress address{{0, 0, 0, 0, 0, 0}};
    EXPECT_EQ(PrintValueToStdString(address), "00-00-00-00-00-00");
    EXPECT_STATUS_OK(address.ReadFromRegion(region));
    EXPECT_NE(address, (EthernetAddress{{0, 0, 0, 0, 0, 0}}));
    EXPECT_EQ(address, (EthernetAddress{{2, 3, 4, 5, 6, 7}}));
    EXPECT_EQ(PrintValueToStdString(address), "02-03-04-05-06-07");
  }

  {
    // ReadFromRegion fails if not enough room.
    mcucore::EepromRegionReader region(eeprom, 0, 2);
    EthernetAddress address;
    EXPECT_THAT(address.ReadFromRegion(region), Not(IsOk()));
  }

  {
    // WriteToRegion fails if not enough room.
    mcucore::EepromRegion region(eeprom, 0, 5);
    EthernetAddress address{{2, 3, 4, 5, 6, 7}};
    EXPECT_THAT(address.WriteToRegion(region), Not(IsOk()));
  }
}

}  // namespace
}  // namespace test
}  // namespace mcunet
