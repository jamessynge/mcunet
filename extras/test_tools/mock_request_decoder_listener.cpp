#include "extras/test_tools/mock_request_decoder_listener.h"

#include <McuCore.h>

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
using ::testing::StartsWith;

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

void MockRequestDecoderListener::OnUnexpectedCallStopDecoding() {
  // If any unexpected call occurs, stop decoding.
  ON_CALL(*this, OnEvent).WillByDefault([](const OnEventData& data) {
    LOG(INFO) << "Unexpected OnEvent, calling StopDecoding.";
    data.StopDecoding();
  });
  ON_CALL(*this, OnCompleteText)
      .WillByDefault([](const OnCompleteTextData& data) {
        LOG(INFO) << "Unexpected OnCompleteText, calling StopDecoding.";
        data.StopDecoding();
      });
  ON_CALL(*this, OnPartialText)
      .WillByDefault([](const OnPartialTextData& data) {
        LOG(INFO) << "Unexpected OnPartialText, calling StopDecoding.";
        data.StopDecoding();
      });
  ON_CALL(*this, OnError).WillByDefault([](const OnErrorData& data) {
    LOG(INFO) << "Unexpected OnError, calling StopDecoding.";
    data.StopDecoding();
  });
}

void ExpectEvent(MockRequestDecoderListener& rdl, const EEvent event) {
  EXPECT_CALL(rdl, OnEvent(IsEvent(event)))
      .WillOnce([](const OnEventData& data) {
        LOG(INFO) << "Expected OnEvent " << data.event;
      })
      .RetiresOnSaturation();
}

// This is a special case of ExpectEvent(rdl, EEvent::kPathEndQueryStart).
void ExpectQueryStart(MockRequestDecoderListener& rdl,
                      bool skip_query_string_decoding) {
  if (skip_query_string_decoding) {
    EXPECT_CALL(rdl, OnEvent(IsEvent(EEvent::kPathEndQueryStart)))
        .WillOnce([](const OnEventData& data) {
          LOG(INFO)
              << "Expected kPathEndQueryStart, skipping query string decoding";
          data.SkipQueryStringDecoding();
        })
        .RetiresOnSaturation();
  } else {
    ExpectEvent(rdl, EEvent::kPathEndQueryStart);
  }
}

void ExpectCompleteText(MockRequestDecoderListener& rdl, const EToken token,
                        const std::string_view expected_text) {
  EXPECT_CALL(rdl, OnCompleteText(IsCompleteText(token, expected_text)))
      .WillOnce([](const OnCompleteTextData& data) {
        LOG(INFO) << "Expected OnCompleteText";
      })
      .RetiresOnSaturation();
}

void ExpectPartialTextMatching(MockRequestDecoderListener& rdl,
                               const EPartialToken token,
                               const std::string expected_text) {
  auto test_info = testing::UnitTest::GetInstance()->current_test_info();
  std::shared_ptr<std::string> accumulator =
      std::make_shared<std::string>("NEVER_SET");
  auto handler = [test_info, accumulator, expected_text](
                     EPartialTokenPosition position,
                     mcucore::StringView text) -> bool {
    if (position == EPartialTokenPosition::kFirst) {
      accumulator->clear();
    }
    *accumulator += std::string(text.data(), text.size());
    if (position == EPartialTokenPosition::kLast) {
      EXPECT_EQ(expected_text, *accumulator);
    } else {
      // The accumulated text should be a prefix of the expected text.
      EXPECT_THAT(expected_text, StartsWith(*accumulator));
    }
    return !test_info->result()->Failed();
  };
  InSequence s;
  EXPECT_CALL(rdl, OnPartialText(IsPartialTextTokenPosition(
                       token, EPartialTokenPosition::kFirst)))
      .WillOnce([handler](const OnPartialTextData& data) {
        if (!handler(data.position, data.text)) {
          // Failed.
          data.StopDecoding();
        }
      })
      .RetiresOnSaturation();
  EXPECT_CALL(rdl, OnPartialText(IsPartialTextTokenPosition(
                       token, EPartialTokenPosition::kMiddle)))
      .WillRepeatedly([handler](const OnPartialTextData& data) {
        if (!handler(data.position, data.text)) {
          // Failed.
          data.StopDecoding();
        }
      });
  EXPECT_CALL(rdl, OnPartialText(IsPartialTextTokenPosition(
                       token, EPartialTokenPosition::kLast)))
      .WillOnce([handler](const OnPartialTextData& data) {
        if (!handler(data.position, data.text)) {
          // Failed.
          data.StopDecoding();
        }
      })
      .RetiresOnSaturation();
}

void ExpectError(MockRequestDecoderListener& rdl, const std::string message) {
  EXPECT_CALL(rdl, OnError(IsError(message)))
      .WillOnce(
          [](const OnErrorData& data) { LOG(INFO) << "Expected OnError"; })
      .RetiresOnSaturation();
}

}  // namespace test
}  // namespace http1
}  // namespace mcunet
