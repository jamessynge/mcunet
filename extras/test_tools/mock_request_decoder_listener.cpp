#include "extras/test_tools/mock_request_decoder_listener.h"

#include <memory>
#include <string>
#include <string_view>

#include "http1/request_decoder.h"
#include "http1/request_decoder_constants.h"
#include "mcucore/extras/test_tools/progmem_string_utils.h"  // IWYU pragma: keep it is actually used
#include "mcucore/extras/test_tools/string_view_utils.h"  // IWYU pragma: keep it is actually used

namespace mcunet {
namespace http1 {
namespace test {
namespace {

using ::testing::AllOf;
using ::testing::Eq;
using ::testing::ExplainMatchResult;
using ::testing::Field;
using ::testing::InSequence;

MATCHER_P(IsEvent, event, "") {
  return ExplainMatchResult(Field(&OnEventData::event, Eq(event)), arg,
                            result_listener);
}

MATCHER_P2(IsCompleteText, token, text, "") {
  return ExplainMatchResult(AllOf(Field(&OnCompleteTextData::token, Eq(token)),
                                  Field(&OnCompleteTextData::text, Eq(text))),
                            arg, result_listener);
}

MATCHER_P2(IsPartialTextTokenPosition, token, position, "") {
  return ExplainMatchResult(
      AllOf(Field(&OnPartialTextData::token, Eq(token)),
            Field(&OnPartialTextData::position, Eq(position))),
      arg, result_listener);
}

MATCHER_P(IsError, message, "") {
  return ExplainMatchResult(Field(&OnErrorData::message, Eq(message)), arg,
                            result_listener);
}

}  // namespace

void ExpectEvent(MockRequestDecoderListener& rdl, const EEvent event) {
  EXPECT_CALL(rdl, OnEvent(IsEvent(event)));
}

void ExpectCompleteText(MockRequestDecoderListener& rdl, const EToken token,
                        const std::string text) {
  EXPECT_CALL(rdl, OnCompleteText(IsCompleteText(token, text)));
}

void ExpectPartialTextMatching(MockRequestDecoderListener& rdl,
                               const EPartialToken token,
                               const std::string text) {
  std::shared_ptr<std::string> accumulator =
      std::make_shared<std::string>("NEVER_SET");
  InSequence s;
  EXPECT_CALL(rdl, OnPartialText(IsPartialTextTokenPosition(
                       token, EPartialTokenPosition::kFirst)))
      .WillOnce([accumulator](const OnPartialTextData& data) {
        *accumulator = std::string(data.text.data(), data.text.size());
      });
  EXPECT_CALL(rdl, OnPartialText(IsPartialTextTokenPosition(
                       token, EPartialTokenPosition::kMiddle)))
      .WillRepeatedly([accumulator](const OnPartialTextData& data) {
        *accumulator += std::string(data.text.data(), data.text.size());
      });
  EXPECT_CALL(rdl, OnPartialText(IsPartialTextTokenPosition(
                       token, EPartialTokenPosition::kLast)))
      .WillOnce([accumulator, text](const OnPartialTextData& data) {
        *accumulator += std::string(data.text.data(), data.text.size());
        EXPECT_EQ(*accumulator, text);
      });
}

void ExpectError(MockRequestDecoderListener& rdl, const std::string message) {
  EXPECT_CALL(rdl, OnError(IsError(message)));
}

}  // namespace test
}  // namespace http1
}  // namespace mcunet
