// Tests of helper functions in mcunet::http1::mcunet_http1_internal.

#include "gtest/gtest.h"
#include "http1/request_decoder.h"

namespace mcunet {
namespace http1 {
namespace mcunet_http1_internal {
namespace test {
namespace {

using ::mcucore::StringView;

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
