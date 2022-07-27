// Tests of helper functions in mcunet::http1::mcunet_http1_internal.

// TODO(jamessynge): Trim down the includes after writing tests.
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "http1/request_decoder.h"
#include "mcucore/extras/host/arduino/wcharacter.h"
#include "mcucore/extras/test_tools/print_to_std_string.h"
#include "mcucore/extras/test_tools/print_value_to_std_string.h"
#include "mcucore/extras/test_tools/sample_printable.h"

namespace mcunet {
namespace http1 {
namespace mcunet_http1_internal {
namespace test {
namespace {

// TODO(jamessynge): Trim down the using declarations after writing tests.
using ::mcucore::PrintValueToStdString;
using mcucore::StringView;
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

TEST(RequestDecoderInternalsTest, isAlphaNumeric) {
  EXPECT_FALSE(isAlphaNumeric(' '));
  auto* test = isAlphaNumeric;
  EXPECT_FALSE(test(' '));
}

TEST(RequestDecoderInternalsTest, IsPChar) {
  EXPECT_FALSE(IsPChar(' '));
  auto* test = IsPChar;
  EXPECT_FALSE(test(' '));
}

TEST(RequestDecoderInternalsTest, IsQueryChar) {
  EXPECT_FALSE(IsQueryChar(' '));
  auto* test = IsQueryChar;
  EXPECT_FALSE(test(' '));
}

TEST(RequestDecoderInternalsTest, FindFirstNotOf) {
  EXPECT_EQ(FindFirstNotOf(StringView(" HTTP/1.1"), IsQueryChar), 0);
}

}  // namespace
}  // namespace test
}  // namespace mcunet_http1_internal
}  // namespace http1
}  // namespace mcunet
