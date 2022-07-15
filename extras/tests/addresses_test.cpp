#include "addresses.h"

#include <McuCore.h>

#include <set>

#include "ethernet_address.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "ip_address.h"
#include "mcucore/extras/host/eeprom/eeprom.h"
#include "mcucore/extras/test_tools/print_value_to_std_string.h"
#include "mcucore/extras/test_tools/status_test_utils.h"

namespace mcunet {
namespace test {
namespace {

using ::mcucore::PrintValueToStdString;
using ::mcucore::test::IsOk;
using ::testing::AllOf;
using ::testing::HasSubstr;
using ::testing::Not;

TEST(AddressesTest, Compare) {
  // EthernetAddress doesn't initialize the bytes unless specified, so we can't
  // expect to get predictable results when comparing two instances that haven't
  // been explicitly initialized... And we'd get a memory sanitizer error.
  EthernetAddress mac1{0, 1, 2, 3, 4, 5};
  EthernetAddress mac2{1, 2, 3, 4, 5, 6};
  IpAddress ip1(10, 0, 0, 5);
  IpAddress ip2(192, 168, 9, 1);

  Addresses addresses{mac1, ip1};
  EXPECT_EQ(addresses, (Addresses{mac1, ip1}));
  EXPECT_NE(addresses, (Addresses{mac2, ip1}));
  EXPECT_NE(addresses, (Addresses{mac1, ip2}));
  EXPECT_NE(addresses, (Addresses{mac2, ip2}));

  // Doesn't matter whether we're dealing with temporaries or not.
  EXPECT_EQ((Addresses{mac2, ip2}), (Addresses{mac2, ip2}));
}

TEST(AddressesTest, WriteAndReadRegion) {
  EEPROMClass eeprom;
  {
    mcucore::EepromRegion region(eeprom, 0, eeprom.length());
    EXPECT_EQ(region.cursor(), 0);
    Addresses addresses;
    addresses.ethernet = EthernetAddress{{0x02, 0x42, 0xc9, 0x3f, 0x40, 0x0f}};
    addresses.ip = IpAddress(10, 1, 2, 3);
    EXPECT_STATUS_OK(addresses.WriteToRegion(region));
    EXPECT_EQ(region.cursor(), 10);
    EXPECT_THAT(PrintValueToStdString(addresses),
                AllOf(HasSubstr("ethernet=02-42-C9-3F-40-0F"),
                      HasSubstr("ip=10.1.2.3")));
  }

  {
    mcucore::EepromRegionReader region(eeprom, 0, eeprom.length());
    Addresses addresses;
    addresses.ethernet = EthernetAddress{{0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00}};
    addresses.ip = IpAddress(255, 0, 255, 0);
    EXPECT_THAT(PrintValueToStdString(addresses),
                AllOf(HasSubstr("ethernet=FF-00-FF-00-FF-00"),
                      HasSubstr("ip=255.0.255.0")));
    EXPECT_STATUS_OK(addresses.ReadFromRegion(region));
    EXPECT_EQ(addresses.ethernet,
              (EthernetAddress{{0x02, 0x42, 0xc9, 0x3f, 0x40, 0x0f}}));
    EXPECT_EQ(addresses.ip, IpAddress(10, 1, 2, 3));
    EXPECT_THAT(PrintValueToStdString(addresses),
                AllOf(HasSubstr("ethernet=02-42-C9-3F-40-0F"),
                      HasSubstr("ip=10.1.2.3")));
  }

  {
    // ReadFromRegion fails if not enough room.
    mcucore::EepromRegionReader region(eeprom, 0, 9);
    Addresses addresses;
    EXPECT_THAT(addresses.ReadFromRegion(region), Not(IsOk()));
  }

  {
    // WriteToRegion fails if not enough room.
    mcucore::EepromRegion region(eeprom, 0, 9);
    Addresses addresses;
    EXPECT_THAT(addresses.WriteToRegion(region), Not(IsOk()));
  }
}

TEST(AddressesTest, WriteAndReadEntry) {
  EEPROMClass eeprom(50);
  auto eeprom_tlv = mcucore::EepromTlv::GetOrDie(eeprom);
  Addresses addresses;

  // No entry to find yet.
  EXPECT_THAT(addresses.ReadEepromEntry(eeprom_tlv),
              mcucore::test::StatusIs(mcucore::StatusCode::kNotFound));

  // Set the values, then write an entry.
  addresses.ethernet = EthernetAddress{{0x02, 0x42, 0xc9, 0x3f, 0x40, 0x0f}};
  addresses.ip = IpAddress(10, 1, 2, 3);
  EXPECT_STATUS_OK(addresses.WriteEepromEntry(eeprom_tlv));

  // Read it back into another instance.
  Addresses addresses2{.ethernet = EthernetAddress{{0, 0, 0, 0, 0, 0}},
                       .ip = IpAddress()};
  EXPECT_NE(addresses, addresses2);
  EXPECT_STATUS_OK(addresses2.ReadEepromEntry(eeprom_tlv));
  EXPECT_EQ(addresses, addresses2);

  // Verify the OuiPrefix.
  OuiPrefix oui_prefix(0x02, 0x42, 0xc9);
  Addresses addresses3{.ethernet = EthernetAddress{{0, 0, 0, 0, 0, 0}},
                       .ip = IpAddress()};
  EXPECT_STATUS_OK(addresses3.ReadEepromEntry(eeprom_tlv, &oui_prefix));
  EXPECT_EQ(addresses, addresses3);

  // Mismatch of prefix is detected.
  oui_prefix = OuiPrefix(0xFF, 0, 0);
  Addresses addresses4{.ethernet = EthernetAddress{{0, 0, 0, 0, 0, 0}},
                       .ip = IpAddress()};
  EXPECT_THAT(
      addresses4.ReadEepromEntry(eeprom_tlv, &oui_prefix),
      mcucore::test::StatusIs(mcucore::StatusCode::kFailedPrecondition));
}

TEST(AddressesTest, GenerateAddresses) {
  OuiPrefix oui_prefix(0x02, 0x04, 0x08);
  std::set<Addresses> generated_addresses;
  for (int i = 0; i < 1000; ++i) {
    Addresses addresses;
    addresses.GenerateAddresses(&oui_prefix);
    addresses.ethernet.HasOuiPrefix(oui_prefix);
    EXPECT_EQ(generated_addresses.insert(addresses).second, true);
  }
}

}  // namespace
}  // namespace test
}  // namespace mcunet
