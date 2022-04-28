#include "ip_address.h"

#include <McuCore.h>

#include <set>

#include "absl/strings/str_cat.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mcucore/extras/test_tools/print_value_to_std_string.h"
#include "mcucore/extras/test_tools/status_test_utils.h"
#include "platform_ethernet.h"

namespace mcunet {
bool operator<(const IpAddress& lhs, const IpAddress& rhs) {
  for (int i = 0; i < 4; ++i) {
    if (lhs[i] < rhs[i]) {
      return true;
    } else if (lhs[i] > rhs[i]) {
      return false;
    }
  }
  return false;  // Equal, not less
}

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

TEST(IpAddressTest, GenerateUniqueAddresses) {
  std::set<IpAddress> addresses;
  // There are fewer than 64K addresses in the link-local range, so we can't
  // expect no collisions if we generate a bunch of addresses. Instead what we
  // do expect 60% percent of them to be unique, which is pretty likely. I
  // tested this with 1000 runs.
  for (int i = 0; i < 100; ++i) {
    IpAddress address;
    address.GenerateAddress();
    addresses.insert(address);
  }
  EXPECT_GT(addresses.size(), 60);
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
