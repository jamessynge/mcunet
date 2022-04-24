#include "ip_address.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mcucore/extras/test_tools/print_value_to_std_string.h"
#include "mcucore/extras/test_tools/status_test_utils.h"

namespace mcunet {
namespace test {
namespace {

using ::mcucore::PrintValueToStdString;
using ::mcucore::test::IsOk;
using ::testing::Not;

MATCHER_P2(IsBetween, a, b,
           absl::StrCat(negation ? "isn't" : "is", " between ",
                        testing::PrintToString(a), " and ",
                        testing::PrintToString(b))) {
  return a <= arg && arg <= b;
}

TEST(IpAddressTest, Streamable) {
  // Can insert into OPrintStream.
  EXPECT_EQ(PrintValueToStdString(IpAddress(192, 168, 9, 1)), "192.168.9.1");
}

TEST(IpAddressTest, GenerateAddress) {
  IpAddress address;
  address.GenerateAddress();
  EXPECT_EQ(address[0], 169);
  EXPECT_EQ(address[1], 254);
  EXPECT_THAT(address[2], IsBetween(1, 254));
}

TEST(IpAddressTest, WriteAndRead) {
  EEPROMClass eeprom;
  {
    mcucore::EepromRegion region(eeprom, 0, eeprom.length());
    EXPECT_EQ(region.cursor(), 0);
    IpAddress address(1, 2, 3, 4);
    EXPECT_STATUS_OK(address.WriteToRegion(region));
    EXPECT_EQ(region.cursor(), 4);
    EXPECT_EQ(PrintValueToStdString(address), "1.2.3.4");
  }

  {
    mcucore::EepromRegionReader region(eeprom, 0, eeprom.length());
    IpAddress address(255, 255, 255, 255);
    EXPECT_EQ(PrintValueToStdString(address), "255.255.255.255");
    EXPECT_STATUS_OK(address.ReadFromRegion(region));
    EXPECT_NE(address, (IpAddress(255, 255, 255, 255)));
    EXPECT_EQ(address, (IpAddress(1, 2, 3, 4)));
    EXPECT_EQ(PrintValueToStdString(address), "1.2.3.4");
  }

  {
    // ReadFromRegion fails if not enough room.
    mcucore::EepromRegionReader region(eeprom, 0, 3);
    IpAddress address;
    EXPECT_THAT(address.ReadFromRegion(region), Not(IsOk()));
  }

  {
    // WriteToRegion fails if not enough room.
    mcucore::EepromRegion region(eeprom, 0, 3);
    IpAddress address(1, 2, 3, 4);
    EXPECT_THAT(address.WriteToRegion(region), Not(IsOk()));
  }
}

}  // namespace
}  // namespace test
}  // namespace mcunet
