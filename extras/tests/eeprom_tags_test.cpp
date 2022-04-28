#include "eeprom_tags.h"

// TODO(jamessynge): Trim down the includes after writing tests.
#include <McuCore.h>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mcucore/extras/test_tools/print_to_std_string.h"
#include "mcucore/extras/test_tools/print_value_to_std_string.h"
#include "mcucore/extras/test_tools/sample_printable.h"
#include "mcucore/extras/test_tools/status_test_utils.h"

namespace mcunet {
namespace test {
namespace {

// TODO(jamessynge): Trim down the using declarations after writing tests.
using ::mcucore::PrintValueToStdString;
using ::mcucore::test::IsOk;
using ::mcucore::test::PrintToStdString;
using ::mcucore::test::SamplePrintable;
using ::testing::AllOf;
using ::testing::AnyNumber;
using ::testing::AnyOf;
using ::testing::Contains;
using ::testing::ContainsRegex;
using ::testing::ElementsAre;
using ::testing::EndsWith;
using ::testing::Eq;
using ::testing::HasSubstr;
using ::testing::InSequence;
using ::testing::IsEmpty;
using ::testing::Mock;
using ::testing::NiceMock;
using ::testing::Not;
using ::testing::Ref;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::SizeIs;
using ::testing::StartsWith;
using ::testing::StrictMock;

TEST(NoFixture_EepromTagsTest, SomeTest) { EXPECT_EQ(1, 1); }

// DO NOT SUBMIT: Remove this comment block.
//
// Useful links:
//
//  * http://go/gunitprimer introduction to gUnit.
//  * http://go/unit-testing-best-practices general best practices.
//  * http://go/gmockguide introduction to gMock.
//  * http://go/matchers a list of gUnit and gMock matchers.
//
//  * http://go/totw/97 on writing simpler tests.
//  * http://go/totw/122 on improving test readability.
//  * http://go/totw/113 on proper usage of ASSERTs and EXPECTs.
//  * http://go/totw/135 on avoiding FRIEND_TEST.
//  * http://go/totw/111 on testing with Time and Clocks.
//
// DO NOT SUBMIT: Remove this comment block.

class EepromTagsTest : public testing::Test {
 protected:
  EepromTagsTest() {}
  void SetUp() override {}
};

TEST_F(EepromTagsTest, SomeTest) { EXPECT_EQ(1, 1); }

}  // namespace
}  // namespace test
}  // namespace mcunet
