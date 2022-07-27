#include "http1/request_decoder.h"

#include <algorithm>
#include <string>
#include <tuple>
#include <vector>

// TODO(jamessynge): Trim down the includes after writing tests.
#include <McuCore.h>

#include "absl/strings/str_cat.h"
#include "extras/test_tools/mock_request_decoder_listener.h"
#include "extras/test_tools/string_utils.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "http1/request_decoder_constants.h"
#include "mcucore/extras/test_tools/print_to_std_string.h"
#include "mcucore/extras/test_tools/print_value_to_std_string.h"
#include "mcucore/extras/test_tools/sample_printable.h"

namespace mcunet {
namespace http1 {
namespace test {
namespace {

// TODO(jamessynge): Trim down the using declarations after writing tests.
using ::mcucore::PrintValueToStdString;
using ::mcucore::StringView;
using ::mcucore::test::PrintToStdString;
using ::mcucore::test::SamplePrintable;
using ::mcunet::test::AppendRemainder;
using ::mcunet::test::GenerateMultipleRequestPartitions;
using ::mcunet::test::SplitEveryN;
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

constexpr const size_t kDecodeBufferSize = 40;

bool TestHasFailed() {
  auto test_info = testing::UnitTest::GetInstance()->current_test_info();
  return test_info->result()->Failed();
}

// Decode the contents of buffer as long as it isn't empty and the decoder isn't
// done.
EDecodeBufferStatus DecodeBuffer(
    RequestDecoder& decoder, std::string& buffer, const bool at_end,
    const size_t max_decode_buffer_size = kDecodeBufferSize) {
  CHECK(!buffer.empty());
  CHECK_GT(max_decode_buffer_size, 0);
  CHECK_LE(max_decode_buffer_size, mcucore::StringView::kMaxSize);

  EDecodeBufferStatus status = EDecodeBufferStatus::kInternalError;
  do {
    // We deliberately copy into another string. This can allow msan or asan to
    // detect when we've read too far.
    const size_t initial_size = std::min(max_decode_buffer_size, buffer.size());
    std::string copy = buffer.substr(0, initial_size);
    mcucore::StringView view(copy.data(), initial_size);

    const bool buffer_is_full = view.size() >= max_decode_buffer_size;
    status = decoder.DecodeBuffer(view, buffer_is_full);

    // Make sure that the decoder only removed the prefix of the view.
    EXPECT_GE(initial_size, view.size());
    const size_t removed_size = initial_size - view.size();
    EXPECT_EQ(copy.data() + removed_size, view.data());

    // Make sure that the decoder didn't modify the passed in buffer.
    EXPECT_EQ(buffer.substr(0, initial_size), copy.substr(0, initial_size));

    // Remove the decoded prefix of buffer.
    buffer.erase(0, initial_size - view.size());

    // Are we done?
    LOG(INFO) << "decoder.DecodeBuffer returned " << status
              << " after removing " << removed_size << " characters.";
    if (status >= EDecodeBufferStatus::kComplete || removed_size == 0) {
      return status;
    }
  } while (!buffer.empty());
  return status;
}

EDecodeBufferStatus ResetAndDecodeFullBuffer(
    RequestDecoder& decoder, RequestDecoderListener* rdl, std::string& buffer,
    const size_t max_decode_buffer_size = kDecodeBufferSize) {
  decoder.Reset();
  if (rdl != nullptr) {
    decoder.SetListener(*rdl);
  }
  return DecodeBuffer(decoder, buffer, true, max_decode_buffer_size);
}

// Apply the decoder to decoding the provided partition of a request. Returns
// the final decode status, the remainder of the last buffer passed in, and
// all the remaining undecoded text.
std::tuple<EDecodeBufferStatus, std::string, std::string>
DecodePartitionedRequest(
    RequestDecoder& decoder, RequestDecoderListener* rdl,
    const std::vector<std::string>& partition,
    const size_t max_decode_buffer_size = kDecodeBufferSize) {
  CHECK_NE(partition.size(), 0);
  CHECK_GT(max_decode_buffer_size, 0);
  CHECK_LE(max_decode_buffer_size, mcucore::StringView::kMaxSize);
  decoder.Reset();
  if (rdl != nullptr) {
    decoder.SetListener(*rdl);
  }
  std::string buffer;
  for (int ndx = 0; ndx < partition.size(); ++ndx) {
    const bool at_end = (ndx + 1) == partition.size();
    buffer += partition[ndx];
    auto status = DecodeBuffer(decoder, buffer, at_end, max_decode_buffer_size);
    if (status >= EDecodeBufferStatus::kComplete || at_end) {
      return {status, buffer, AppendRemainder(buffer, partition, ndx + 1)};
    }
  }
  return {EDecodeBufferStatus::kNeedMoreInput, buffer, buffer};
}

TEST(RequestDecoderTest, LogSizes) {
  LOG(INFO) << "sizeof(RequestDecoderListener) "
            << sizeof(RequestDecoderListener);
  LOG(INFO) << "sizeof(RequestDecoder) " << sizeof(RequestDecoder);
}

TEST(RequestDecoderTest, UnusedDecoder) { RequestDecoder decoder; }

TEST(RequestDecoderTest, ResetOnly) {
  RequestDecoder decoder;
  decoder.Reset();
}

TEST(RequestDecoderTest, SetListenerOnly) {
  StrictMock<MockRequestDecoderListener> rdl;
  RequestDecoder decoder;
  decoder.SetListener(rdl);
}

TEST(RequestDecoderTest, ResetRequired) {
  StrictMock<MockRequestDecoderListener> rdl;
  RequestDecoder decoder;
  decoder.SetListener(rdl);
  const StringView original("GET ");
  auto buffer = original;
  EXPECT_EQ(decoder.DecodeBuffer(buffer, false),
            EDecodeBufferStatus::kInternalError);
  EXPECT_EQ(buffer, original);
}

TEST(RequestDecoderTest, SmallestHomePageRequest) {
  RequestDecoder decoder;
  const char kRequestHeader[] =
      "GET / HTTP/1.1\r\n"
      "\r\n";
  const char kOptionalBody[] = "NotAHeaderName:NotAHeaderValue\r\n\r\n";

  for (const auto body : {"", kOptionalBody}) {
    for (auto partition : GenerateMultipleRequestPartitions(
             absl::StrCat(kRequestHeader, body))) {
      LOG(INFO) << "\n"
                << "----------------------------------------"
                << "----------------------------------------";
      StrictMock<MockRequestDecoderListener> rdl;
      InSequence s;
      EXPECT_CALL(rdl, OnCompleteText(EToken::kHttpMethod, Eq("GET")));
      EXPECT_CALL(rdl, OnEvent(EEvent::kPathStart));
      EXPECT_CALL(rdl, OnEvent(EEvent::kPathAndUrlEnd));
      EXPECT_CALL(rdl, OnEvent(EEvent::kHttpVersion1_1));
      EXPECT_CALL(rdl, OnEnd);

      const auto [status, buffer, remainder] =
          DecodePartitionedRequest(decoder, &rdl, partition);
      EXPECT_EQ(status, EDecodeBufferStatus::kComplete);
      EXPECT_THAT(body, StartsWith(buffer));
      EXPECT_THAT(remainder, body);
      if (TestHasFailed()) {
        break;
      }
    }
  }
}

TEST(RequestDecoderTest, SmallestDeviceApiGetRequest) {
  RequestDecoder decoder;
  const char kRequestHeader[] =
      "OPTIONS /api/v1/safetymonitor/0/issafe HTTP/1.1\r\n"
      "\r\n";
  const char kOptionalBody[] = "NotAHeaderName:NotAHeaderValue\r\n\r\n";

  for (const auto body : {"", kOptionalBody}) {
    for (auto partition : GenerateMultipleRequestPartitions(
             absl::StrCat(kRequestHeader, body))) {
      LOG(INFO) << "\n"
                << "----------------------------------------"
                << "----------------------------------------";
      StrictMock<MockRequestDecoderListener> rdl;
      InSequence s;
      EXPECT_CALL(rdl, OnCompleteText(EToken::kHttpMethod, Eq("OPTIONS")));
      EXPECT_CALL(rdl, OnEvent(EEvent::kPathStart));
      EXPECT_CALL(rdl, OnCompleteText(EToken::kPathSegment, Eq("api")));
      EXPECT_CALL(rdl, OnEvent(EEvent::kPathSeparator));
      EXPECT_CALL(rdl, OnCompleteText(EToken::kPathSegment, Eq("v1")));
      EXPECT_CALL(rdl, OnEvent(EEvent::kPathSeparator));
      EXPECT_CALL(rdl,
                  OnCompleteText(EToken::kPathSegment, Eq("safetymonitor")));
      EXPECT_CALL(rdl, OnEvent(EEvent::kPathSeparator));
      EXPECT_CALL(rdl, OnCompleteText(EToken::kPathSegment, Eq("0")));
      EXPECT_CALL(rdl, OnEvent(EEvent::kPathSeparator));
      EXPECT_CALL(rdl, OnCompleteText(EToken::kPathSegment, Eq("issafe")));
      EXPECT_CALL(rdl, OnEvent(EEvent::kPathAndUrlEnd));
      EXPECT_CALL(rdl, OnEvent(EEvent::kHttpVersion1_1));
      EXPECT_CALL(rdl, OnEnd);

      const auto [status, buffer, remainder] =
          DecodePartitionedRequest(decoder, &rdl, partition);
      EXPECT_EQ(status, EDecodeBufferStatus::kComplete);
      EXPECT_THAT(body, StartsWith(buffer));
      EXPECT_THAT(remainder, body);
      if (TestHasFailed()) {
        break;
      }
    }
  }
}

TEST(RequestDecoderTest, RequestTargetEndsWithSlash) {
  RequestDecoder decoder;
  const char kRequestHeader[] =
      "HEAD /setup/ HTTP/1.1\r\n"
      "Host: some.name \r\n"
      "\r\n";
  const char kOptionalBody[] = "NotAHeaderName:NotAHeaderValue\r\n\r\n";

  for (const auto body : {"", kOptionalBody}) {
    for (auto partition : GenerateMultipleRequestPartitions(
             absl::StrCat(kRequestHeader, body))) {
      LOG(INFO) << "\n"
                << "----------------------------------------"
                << "----------------------------------------";
      StrictMock<MockRequestDecoderListener> rdl;
      InSequence s;
      EXPECT_CALL(rdl, OnCompleteText(EToken::kHttpMethod, Eq("HEAD")));
      EXPECT_CALL(rdl, OnEvent(EEvent::kPathStart));
      EXPECT_CALL(rdl, OnCompleteText(EToken::kPathSegment, Eq("setup")));
      EXPECT_CALL(rdl, OnEvent(EEvent::kPathSeparator));
      EXPECT_CALL(rdl, OnEvent(EEvent::kPathAndUrlEnd));
      EXPECT_CALL(rdl, OnEvent(EEvent::kHttpVersion1_1));
      EXPECT_CALL(rdl, OnCompleteText(EToken::kHeaderName, Eq("Host")));
      EXPECT_CALL(rdl, OnCompleteText(EToken::kHeaderValue, Eq("some.name")));
      EXPECT_CALL(rdl, OnEnd);

      const auto [status, buffer, remainder] =
          DecodePartitionedRequest(decoder, &rdl, partition);
      EXPECT_EQ(status, EDecodeBufferStatus::kComplete);
      EXPECT_THAT(body, StartsWith(buffer));
      EXPECT_THAT(remainder, body);
      if (TestHasFailed()) {
        break;
      }
    }
  }
}

TEST(RequestDecoderTest, RootPathPlusEmptyQueryString) {
  RequestDecoder decoder;
  const char kRequestHeader[] =
      "POST /? HTTP/1.1\r\n"
      "Host:some name\r\n"
      "\r\n";
  const char kOptionalBody[] = "NotAHeaderName:NotAHeaderValue\r\n\r\n";

  for (const auto body : {"", kOptionalBody}) {
    for (auto partition : GenerateMultipleRequestPartitions(
             absl::StrCat(kRequestHeader, body))) {
      LOG(INFO) << "\n"
                << "----------------------------------------"
                << "----------------------------------------";
      StrictMock<MockRequestDecoderListener> rdl;
      InSequence s;
      EXPECT_CALL(rdl, OnCompleteText(EToken::kHttpMethod, Eq("POST")));
      EXPECT_CALL(rdl, OnEvent(EEvent::kPathStart));
      EXPECT_CALL(rdl, OnEvent(EEvent::kPathEndQueryStart));
      EXPECT_CALL(rdl, OnEvent(EEvent::kQueryAndUrlEnd));
      EXPECT_CALL(rdl, OnEvent(EEvent::kHttpVersion1_1));
      EXPECT_CALL(rdl, OnCompleteText(EToken::kHeaderName, Eq("Host")));
      EXPECT_CALL(rdl, OnCompleteText(EToken::kHeaderValue, Eq("some name")));
      EXPECT_CALL(rdl, OnEnd);

      const auto [status, buffer, remainder] =
          DecodePartitionedRequest(decoder, &rdl, partition);
      EXPECT_EQ(status, EDecodeBufferStatus::kComplete);
      EXPECT_THAT(body, StartsWith(buffer));
      EXPECT_THAT(remainder, body);
      if (TestHasFailed()) {
        break;
      }
    }
  }
}

}  // namespace
}  // namespace test
}  // namespace http1
}  // namespace mcunet
