#include "http1/request_decoder.h"

#include <McuCore.h>
#include <ctype.h>

#include <algorithm>
#include <cctype>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "absl/strings/str_cat.h"
#include "extras/test_tools/collapsing_request_decoder_listener.h"
#include "extras/test_tools/mock_request_decoder_listener.h"
#include "extras/test_tools/string_utils.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "http1/request_decoder_constants.h"

namespace mcunet {
namespace http1 {

std::ostream& operator<<(std::ostream& strm, const OnCompleteTextData& data) {
  strm << "{token=" << data.token << "}";
  return strm;
}

namespace test {
namespace {

using ::mcucore::StringView;
using ::mcunet::http1::mcunet_http1_internal::IsFieldContent;
using ::mcunet::http1::mcunet_http1_internal::IsPChar;
using ::mcunet::http1::mcunet_http1_internal::IsQueryChar;
using ::mcunet::http1::mcunet_http1_internal::IsTokenChar;
using ::mcunet::test::AllCharsExcept;
using ::mcunet::test::AllRegisteredMethodNames;
using ::mcunet::test::AppendRemainder;
using ::mcunet::test::EveryChar;
using ::mcunet::test::GenerateInvalidPercentEncodedChar;
using ::mcunet::test::GenerateMultipleRequestPartitions;
using ::mcunet::test::PercentEncodeAllChars;
using ::testing::InSequence;
using ::testing::InvokeWithoutArgs;
using ::testing::Mock;
using ::testing::StartsWith;
using ::testing::StrictMock;

constexpr char kLongestMatchedLiteral[] = "HTTP/1.1\r\n";
constexpr auto kMinimumDecodeBufferSize =
    std::string_view(kLongestMatchedLiteral).size();
constexpr size_t kDecodeBufferSize = 40;

std::vector<std::vector<std::string>> GenerateMultipleRequestPartitions(
    const std::string& full_request) {
  return GenerateMultipleRequestPartitions(
      full_request, kDecodeBufferSize,
      std::string_view(kLongestMatchedLiteral).size());
}

bool TestHasFailed() {
  auto test_info = testing::UnitTest::GetInstance()->current_test_info();
  return test_info->result()->Failed();
}

// Decode the contents of buffer as long as it isn't empty and the decoder isn't
// done.
EDecodeBufferStatus DecodeBuffer(
    RequestDecoder& decoder, RequestDecoderListener& listener,
    std::string& buffer, const bool at_end,
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
    status = decoder.DecodeBuffer(view, listener, buffer_is_full);

    // Make sure that the decoder only removed the prefix of the view. The one
    // exception is when the decoder cleared the view.
    EXPECT_GE(initial_size, view.size());
    const size_t removed_size = initial_size - view.size();
    if (view.data() == nullptr) {
      EXPECT_EQ(view.size(), 0);
    } else {
      EXPECT_EQ(copy.data() + removed_size, view.data());
    }

    // Make sure that the decoder didn't modify the passed in buffer.
    EXPECT_EQ(buffer.substr(0, initial_size), copy.substr(0, initial_size));

    // Remove the decoded prefix of buffer.
    buffer.erase(0, removed_size);

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
    RequestDecoder& decoder, RequestDecoderListener& listener,
    std::string& buffer,
    const size_t max_decode_buffer_size = kDecodeBufferSize) {
  decoder.Reset();
  return DecodeBuffer(decoder, listener, buffer, true, max_decode_buffer_size);
}

// Apply the decoder to decoding the provided partition of a request. Returns
// the final decode status, the remainder of the last buffer passed in, and
// all the remaining undecoded text.
std::tuple<EDecodeBufferStatus, std::string, std::string>
DecodePartitionedRequest(
    RequestDecoder& decoder, RequestDecoderListener& listener,
    const std::vector<std::string>& partition,
    const size_t max_decode_buffer_size = kDecodeBufferSize) {
  CHECK_NE(partition.size(), 0);
  CHECK_GT(max_decode_buffer_size, 0);
  CHECK_LE(max_decode_buffer_size, mcucore::StringView::kMaxSize);
  decoder.Reset();
  std::string buffer;
  for (int ndx = 0; ndx < partition.size(); ++ndx) {
    const bool at_end = (ndx + 1) == partition.size();
    buffer += partition[ndx];
    auto status =
        DecodeBuffer(decoder, listener, buffer, at_end, max_decode_buffer_size);
    if (status >= EDecodeBufferStatus::kComplete || at_end) {
      return {status, buffer, AppendRemainder(buffer, partition, ndx + 1)};
    }
  }
  return {EDecodeBufferStatus::kNeedMoreInput, buffer, buffer};
}

TEST(RequestDecoderTest, ShortRequest) {
  RequestDecoder decoder;
  const char kOptionalBody[] = "NotAHeaderName:NotAHeaderValue\r\n\r\n";
  for (const auto body : {"", kOptionalBody}) {
    for (const auto method : AllRegisteredMethodNames()) {
      const auto request =
          absl::StrCat(method, " / HTTP/1.1\r\n", "\r\n", body);
      for (auto partition : GenerateMultipleRequestPartitions(request)) {
        LOG(INFO) << "\n"
                  << "----------------------------------------"
                  << "----------------------------------------";
        StrictMock<MockRequestDecoderListener> rdl;
        InSequence s;
        ExpectCompleteText(rdl, EToken::kHttpMethod, method);
        ExpectEvent(rdl, EEvent::kPathStart);
        ExpectEvent(rdl, EEvent::kPathEnd);
        ExpectEvent(rdl, EEvent::kHttpVersion1_1);
        ExpectEvent(rdl, EEvent::kHeadersEnd);

        const auto [status, buffer, remainder] =
            DecodePartitionedRequest(decoder, rdl, partition);
        EXPECT_EQ(status, EDecodeBufferStatus::kComplete);
        EXPECT_THAT(body, StartsWith(buffer));
        EXPECT_THAT(remainder, body);
        if (TestHasFailed()) {
          return;
        }
      }
    }
  }
}

TEST(RequestDecoderTest, DeviceApiGetRequest) {
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
      CollapsingRequestDecoderListener crdl(rdl);
      InSequence s;
      ExpectCompleteText(rdl, EToken::kHttpMethod, "OPTIONS");
      ExpectEvent(rdl, EEvent::kPathStart);
      ExpectCompleteText(rdl, EToken::kPathSegment, "api");
      ExpectEvent(rdl, EEvent::kPathSeparator);
      ExpectCompleteText(rdl, EToken::kPathSegment, "v1");
      ExpectEvent(rdl, EEvent::kPathSeparator);
      ExpectCompleteText(rdl, EToken::kPathSegment, "safetymonitor");
      ExpectEvent(rdl, EEvent::kPathSeparator);
      ExpectCompleteText(rdl, EToken::kPathSegment, "0");
      ExpectEvent(rdl, EEvent::kPathSeparator);
      ExpectCompleteText(rdl, EToken::kPathSegment, "issafe");
      ExpectEvent(rdl, EEvent::kPathEnd);
      ExpectEvent(rdl, EEvent::kHttpVersion1_1);
      ExpectEvent(rdl, EEvent::kHeadersEnd);

      const auto [status, buffer, remainder] =
          DecodePartitionedRequest(decoder, crdl, partition);
      EXPECT_EQ(status, EDecodeBufferStatus::kComplete);
      EXPECT_THAT(body, StartsWith(buffer));
      EXPECT_THAT(remainder, body);
      if (TestHasFailed()) {
        return;
      }
    }
  }
}

TEST(RequestDecoderTest, RequestTargetEndsWithSlash) {
  RequestDecoder decoder;
  const char kRequestHeader[] =
      "HEAD /setup/ HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "\r\n";
  const char kOptionalBody[] = "NotAHeaderName:NotAHeaderValue\r\n\r\n";

  for (const auto body : {"", kOptionalBody}) {
    for (auto partition : GenerateMultipleRequestPartitions(
             absl::StrCat(kRequestHeader, body))) {
      LOG(INFO) << "\n"
                << "----------------------------------------"
                << "----------------------------------------";
      StrictMock<MockRequestDecoderListener> rdl;
      CollapsingRequestDecoderListener crdl(rdl);
      InSequence s;
      ExpectCompleteText(rdl, EToken::kHttpMethod, "HEAD");
      ExpectEvent(rdl, EEvent::kPathStart);
      ExpectCompleteText(rdl, EToken::kPathSegment, "setup");
      ExpectEvent(rdl, EEvent::kPathSeparator);
      ExpectEvent(rdl, EEvent::kPathEnd);
      ExpectEvent(rdl, EEvent::kHttpVersion1_1);
      ExpectCompleteText(rdl, EToken::kHeaderName, "Host");
      ExpectCompleteText(rdl, EToken::kHeaderValue, "example.com");
      ExpectEvent(rdl, EEvent::kHeadersEnd);

      const auto [status, buffer, remainder] =
          DecodePartitionedRequest(decoder, crdl, partition);
      EXPECT_EQ(status, EDecodeBufferStatus::kComplete);
      EXPECT_THAT(body, StartsWith(buffer));
      EXPECT_THAT(remainder, body);
      if (TestHasFailed()) {
        return;
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

  for (const bool skip_query_string_decoding : {false, true}) {
    for (const auto body : {"", kOptionalBody}) {
      for (auto partition : GenerateMultipleRequestPartitions(
               absl::StrCat(kRequestHeader, body))) {
        LOG(INFO) << "\n"
                  << "----------------------------------------"
                  << "----------------------------------------";
        StrictMock<MockRequestDecoderListener> rdl;
        CollapsingRequestDecoderListener crdl(rdl);
        InSequence s;
        ExpectCompleteText(rdl, EToken::kHttpMethod, "POST");
        ExpectEvent(rdl, EEvent::kPathStart);
        ExpectQueryStart(rdl, skip_query_string_decoding);
        ExpectEvent(rdl, EEvent::kHttpVersion1_1);
        ExpectCompleteText(rdl, EToken::kHeaderName, "Host");
        ExpectCompleteText(rdl, EToken::kHeaderValue, "some name");
        ExpectEvent(rdl, EEvent::kHeadersEnd);

        const auto [status, buffer, remainder] =
            DecodePartitionedRequest(decoder, crdl, partition);
        EXPECT_EQ(status, EDecodeBufferStatus::kComplete);
        EXPECT_THAT(body, StartsWith(buffer));
        EXPECT_THAT(remainder, body);
        if (TestHasFailed()) {
          return;
        }
      }
    }
  }
}

TEST(RequestDecoderTest, MatchVariousQueryParams) {
  RequestDecoder decoder;
  const char kRequestHeader[] =
      "DELETE /a/?n&x=y%26z&a%3D%26=b?c+d%0D%0A&&1=2 HTTP/1.1\r\n"
      "\r\n";
  const char kOptionalBody[] = "NotAHeaderName:NotAHeaderValue\r\n\r\n";

  for (const auto body : {"", kOptionalBody}) {
    for (auto partition : GenerateMultipleRequestPartitions(
             absl::StrCat(kRequestHeader, body))) {
      LOG(INFO) << "\n"
                << "----------------------------------------"
                << "----------------------------------------";
      StrictMock<MockRequestDecoderListener> rdl;
      CollapsingRequestDecoderListener crdl(rdl);
      InSequence s;
      ExpectCompleteText(rdl, EToken::kHttpMethod, "DELETE");
      ExpectEvent(rdl, EEvent::kPathStart);
      ExpectCompleteText(rdl, EToken::kPathSegment, "a");
      ExpectEvent(rdl, EEvent::kPathSeparator);
      ExpectEvent(rdl, EEvent::kPathEndQueryStart);
      ExpectCompleteText(rdl, EToken::kParamName, "n");
      ExpectCompleteText(rdl, EToken::kParamName, "x");
      ExpectCompleteText(rdl, EToken::kParamValue, "y&z");
      ExpectCompleteText(rdl, EToken::kParamName, "a=&");
      ExpectCompleteText(rdl, EToken::kParamValue, "b?c d\r\n");
      ExpectCompleteText(rdl, EToken::kParamName, "1");
      ExpectCompleteText(rdl, EToken::kParamValue, "2");
      ExpectEvent(rdl, EEvent::kHttpVersion1_1);
      ExpectEvent(rdl, EEvent::kHeadersEnd);

      const auto [status, buffer, remainder] =
          DecodePartitionedRequest(decoder, crdl, partition);
      EXPECT_EQ(status, EDecodeBufferStatus::kComplete);
      EXPECT_THAT(body, StartsWith(buffer));
      EXPECT_THAT(remainder, body);
      if (TestHasFailed()) {
        return;
      }
    }
  }
}

TEST(RequestDecoderTest, MatchVariousHeaderForms) {
  RequestDecoder decoder;
  const char kRequestHeader[] =
      "PUT / HTTP/1.1\r\n"
      "name: some.text \r\n"
      "Name: text:with/interesting \r\n"
      "NAME: interesting\tcharacters \r\n"
      "N-A-M-E: characters&in?it \r\n"
      "\r\n";
  const char kOptionalBody[] = "NotAHeaderName:NotAHeaderValue\r\n\r\n";

  for (const auto body : {"", kOptionalBody}) {
    for (auto partition : GenerateMultipleRequestPartitions(
             absl::StrCat(kRequestHeader, body))) {
      LOG(INFO) << "\n"
                << "----------------------------------------"
                << "----------------------------------------";
      StrictMock<MockRequestDecoderListener> rdl;
      CollapsingRequestDecoderListener crdl(rdl);
      InSequence s;
      ExpectCompleteText(rdl, EToken::kHttpMethod, "PUT");
      ExpectEvent(rdl, EEvent::kPathStart);
      ExpectEvent(rdl, EEvent::kPathEnd);
      ExpectEvent(rdl, EEvent::kHttpVersion1_1);
      ExpectCompleteText(rdl, EToken::kHeaderName, "name");
      ExpectCompleteText(rdl, EToken::kHeaderValue, "some.text");
      ExpectCompleteText(rdl, EToken::kHeaderName, "Name");
      ExpectCompleteText(rdl, EToken::kHeaderValue, "text:with/interesting");
      ExpectCompleteText(rdl, EToken::kHeaderName, "NAME");
      ExpectCompleteText(rdl, EToken::kHeaderValue, "interesting\tcharacters");
      ExpectCompleteText(rdl, EToken::kHeaderName, "N-A-M-E");
      ExpectCompleteText(rdl, EToken::kHeaderValue, "characters&in?it");
      ExpectEvent(rdl, EEvent::kHeadersEnd);

      const auto [status, buffer, remainder] =
          DecodePartitionedRequest(decoder, crdl, partition);
      EXPECT_EQ(status, EDecodeBufferStatus::kComplete);
      EXPECT_THAT(body, StartsWith(buffer));
      EXPECT_THAT(remainder, body);
      if (TestHasFailed()) {
        return;
      }
    }
  }
}

TEST(RequestDecoderTest, PercentEncodedPathSegment) {
  RequestDecoder decoder;
  const char kRequestHeader[] =
      "GET /%2F%Fa%c9%3f/ HTTP/1.1\r\n"
      "\r\n";
  const char kOptionalBody[] = "NotAHeaderName:NotAHeaderValue\r\n\r\n";

  for (const auto body : {"", kOptionalBody}) {
    for (auto partition : GenerateMultipleRequestPartitions(
             absl::StrCat(kRequestHeader, body))) {
      LOG(INFO) << "\n"
                << "----------------------------------------"
                << "----------------------------------------";
      StrictMock<MockRequestDecoderListener> rdl;
      CollapsingRequestDecoderListener crdl(rdl);
      InSequence s;
      ExpectCompleteText(rdl, EToken::kHttpMethod, "GET");
      ExpectEvent(rdl, EEvent::kPathStart);
      ExpectCompleteText(rdl, EToken::kPathSegment, "\x2f\xfA\xC9?");
      ExpectEvent(rdl, EEvent::kPathSeparator);
      ExpectEvent(rdl, EEvent::kPathEnd);
      ExpectEvent(rdl, EEvent::kHttpVersion1_1);
      ExpectEvent(rdl, EEvent::kHeadersEnd);

      const auto [status, buffer, remainder] =
          DecodePartitionedRequest(decoder, crdl, partition);
      EXPECT_EQ(status, EDecodeBufferStatus::kComplete);
      EXPECT_THAT(body, StartsWith(buffer));
      EXPECT_THAT(remainder, body);
      if (TestHasFailed()) {
        return;
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// Tests of the ability to handle oversize text.

TEST(RequestDecoderTest, LongPathSegments) {
  const auto kSegment1 = std::string(kDecodeBufferSize, 'X');
  const auto kSegment2 = std::string(kDecodeBufferSize + 1, 'Y');
  const auto path = absl::StrCat("/", kSegment1, "/", kSegment2);
  const char kRestOfRequest[] =
      " HTTP/1.1\r\n"
      "\r\n";
  RequestDecoder decoder;
  for (const auto slash_at_end : {true, false}) {
    for (const auto query_after_path : {true, false}) {
      const auto request =
          absl::StrCat("GET ", path, (slash_at_end ? "/" : ""),
                       (query_after_path ? "?foo=bar" : ""), kRestOfRequest);
      for (auto partition : GenerateMultipleRequestPartitions(request)) {
        LOG(INFO) << "\n"
                  << "----------------------------------------"
                  << "----------------------------------------";
        std::string segment1, segment2;
        StrictMock<MockRequestDecoderListener> rdl;
        CollapsingRequestDecoderListener crdl(rdl);
        InSequence s;
        ExpectCompleteText(rdl, EToken::kHttpMethod, "GET");
        ExpectEvent(rdl, EEvent::kPathStart);
        ExpectCompleteText(rdl, EToken::kPathSegment, kSegment1);
        ExpectEvent(rdl, EEvent::kPathSeparator);
        ExpectCompleteText(rdl, EToken::kPathSegment, kSegment2);
        if (slash_at_end) {
          ExpectEvent(rdl, EEvent::kPathSeparator);
        }
        if (query_after_path) {
          ExpectEvent(rdl, EEvent::kPathEndQueryStart);
          ExpectCompleteText(rdl, EToken::kParamName, "foo");
          ExpectCompleteText(rdl, EToken::kParamValue, "bar");
        } else {
          ExpectEvent(rdl, EEvent::kPathEnd);
        }
        ExpectEvent(rdl, EEvent::kHttpVersion1_1);
        ExpectEvent(rdl, EEvent::kHeadersEnd);

        const auto [status, buffer, remainder] =
            DecodePartitionedRequest(decoder, crdl, partition);
        EXPECT_EQ(status, EDecodeBufferStatus::kComplete);
        EXPECT_EQ(buffer, "");
        EXPECT_EQ(remainder, "");
        if (TestHasFailed()) {
          return;
        }
      }
    }
  }
}

TEST(RequestDecoderTest, LongPercentEncodedPathSegments) {
  RequestDecoder decoder;
  const std::string segment1 = EveryChar();
  const std::string encoded_segment1 = test::PercentEncodeAllChars(segment1);
  const std::string segment2 = std::string(segment1.rbegin(), segment1.rend());
  const std::string encoded_segment2 = test::PercentEncodeAllChars(segment2);
  for (const auto slash_at_end : {true, false}) {
    const auto request =
        absl::StrCat("POST /", encoded_segment1, "/", encoded_segment2,
                     (slash_at_end ? "/" : ""), " HTTP/1.1\r\n\r\n");
    std::string buffer = request;
    LOG(INFO) << "\n"
              << "----------------------------------------"
              << "----------------------------------------";
    StrictMock<MockRequestDecoderListener> rdl;
    CollapsingRequestDecoderListener crdl(rdl);
    InSequence s;
    ExpectCompleteText(rdl, EToken::kHttpMethod, "POST");
    ExpectEvent(rdl, EEvent::kPathStart);
    ExpectPartialTextMatching(rdl, EPartialToken::kPathSegment, segment1);
    ExpectEvent(rdl, EEvent::kPathSeparator);
    ExpectPartialTextMatching(rdl, EPartialToken::kPathSegment, segment2);
    if (slash_at_end) {
      ExpectEvent(rdl, EEvent::kPathSeparator);
    }
    ExpectEvent(rdl, EEvent::kPathEnd);
    ExpectEvent(rdl, EEvent::kHttpVersion1_1);
    ExpectEvent(rdl, EEvent::kHeadersEnd);

    const auto status = ResetAndDecodeFullBuffer(decoder, crdl, buffer);
    EXPECT_EQ(status, EDecodeBufferStatus::kComplete);
    EXPECT_EQ(buffer, "");
    if (TestHasFailed()) {
      return;
    }
  }
}

TEST(RequestDecoderTest, LongRawQueryString) {
  RequestDecoder decoder;
  for (const auto query_string : {std::string(kDecodeBufferSize, 'Q'),
                                  std::string(kDecodeBufferSize + 1, 'q'),
                                  std::string(kDecodeBufferSize * 2, '?')}) {
    for (const std::string_view path : {"/", "/p", "/p/"}) {
      const auto request = absl::StrCat("GET ", path, "?", query_string,
                                        " HTTP/1.1\r\n", "\r\n");
      for (auto partition : GenerateMultipleRequestPartitions(request)) {
        LOG(INFO) << "\n"
                  << "----------------------------------------"
                  << "----------------------------------------";
        StrictMock<MockRequestDecoderListener> rdl;
        CollapsingRequestDecoderListener crdl(rdl);
        InSequence s;
        ExpectCompleteText(rdl, EToken::kHttpMethod, "GET");
        ExpectEvent(rdl, EEvent::kPathStart);
        if (path.size() > 1) {
          ExpectCompleteText(rdl, EToken::kPathSegment, "p");
          if (path.size() > 2) {
            ExpectEvent(rdl, EEvent::kPathSeparator);
          }
        }
        ExpectQueryStart(rdl, /*skip_query_string_decoding=*/true);
        ExpectPartialTextMatching(rdl, EPartialToken::kRawQueryString,
                                  query_string);
        ExpectEvent(rdl, EEvent::kHttpVersion1_1);
        ExpectEvent(rdl, EEvent::kHeadersEnd);

        const auto [status, buffer, remainder] =
            DecodePartitionedRequest(decoder, crdl, partition);
        EXPECT_EQ(status, EDecodeBufferStatus::kComplete);
        EXPECT_EQ(buffer, "");
        EXPECT_EQ(remainder, "");
        if (TestHasFailed()) {
          return;
        }
      }
    }
  }
}

TEST(RequestDecoderTest, LongHeaderNamesOrValues) {
  const char kStartOfRequest[] = "GET / HTTP/1.1\r\n";
  const auto kName1 = std::string(kDecodeBufferSize, 'X');
  const auto kValue1 = std::string(kDecodeBufferSize, 'x');
  const auto kName2 = std::string(kDecodeBufferSize + 1, 'Y');
  const auto kValue2a = std::string(kDecodeBufferSize / 2, 'y');
  const auto kValue2b = std::string(2 + kDecodeBufferSize / 2, 'a');
  const auto kValue2 =
      absl::StrCat(kValue2a, std::string(kDecodeBufferSize, ' '), kValue2b);
  const auto kRequest =
      absl::StrCat(kStartOfRequest, kName1, ":", kValue1, "\r\n", kName2, ": ",
                   kValue2, "\r\n", "\r\n");
  RequestDecoder decoder;
  for (auto partition : GenerateMultipleRequestPartitions(kRequest)) {
    LOG(INFO) << "\n"
              << "----------------------------------------"
              << "----------------------------------------";
    StrictMock<MockRequestDecoderListener> rdl;
    InSequence s;
    ExpectCompleteText(rdl, EToken::kHttpMethod, "GET");
    ExpectEvent(rdl, EEvent::kPathStart);
    ExpectEvent(rdl, EEvent::kPathEnd);
    ExpectEvent(rdl, EEvent::kHttpVersion1_1);
    ExpectPartialTextMatching(rdl, EPartialToken::kHeaderName, kName1);
    ExpectPartialTextMatching(rdl, EPartialToken::kHeaderValue, kValue1);
    ExpectPartialTextMatching(rdl, EPartialToken::kHeaderName, kName2);
    ExpectPartialTextMatching(rdl, EPartialToken::kHeaderValue, kValue2);
    ExpectEvent(rdl, EEvent::kHeadersEnd);

    const auto [status, buffer, remainder] =
        DecodePartitionedRequest(decoder, rdl, partition);
    EXPECT_EQ(status, EDecodeBufferStatus::kComplete);
    EXPECT_EQ(buffer, "");
    EXPECT_EQ(remainder, "");
    if (TestHasFailed()) {
      return;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// Tests of the ability to detect and reject malformed requests.

TEST(RequestDecoderTest, InvalidMethodStart) {
  RequestDecoder decoder;
  for (const char c :
       AllCharsExcept([](char c) { return std::isupper(c) || c == '-'; })) {
    LOG(INFO) << "\n"
              << "----------------------------------------"
              << "----------------------------------------";
    const auto request = std::string(1, c) +
                         "GET / HTTP/1.1\r\n"
                         "\r\n";
    std::string buffer = request;
    StrictMock<MockRequestDecoderListener> rdl;
    CollapsingRequestDecoderListener crdl(rdl);
    ExpectError(rdl, "Invalid HTTP method start");

    const auto status = ResetAndDecodeFullBuffer(decoder, crdl, buffer);
    EXPECT_EQ(status, EDecodeBufferStatus::kIllFormed);
    EXPECT_EQ(buffer, request);
    if (TestHasFailed()) {
      return;
    }
  }
}

TEST(RequestDecoderTest, InvalidMethodEnd) {
  RequestDecoder decoder;
  for (const auto space_before_path : {true, false}) {
    for (const char c : AllCharsExcept(
             [](char c) { return std::isupper(c) || c == '-' || c == ' '; })) {
      LOG(INFO) << "\n"
                << "----------------------------------------"
                << "----------------------------------------";
      const auto corrupt =
          absl::StrCat(std::string(1, c), (space_before_path ? " " : ""),
                       "/ HTTP/1.1\r\n"
                       "\r\n");
      const auto request = absl::StrCat("GET", corrupt);
      std::string buffer = request;
      StrictMock<MockRequestDecoderListener> rdl;
      CollapsingRequestDecoderListener crdl(rdl);
      ExpectError(rdl, "Invalid HTTP method end");

      const auto status = ResetAndDecodeFullBuffer(decoder, crdl, buffer);
      EXPECT_EQ(status, EDecodeBufferStatus::kIllFormed);
      EXPECT_EQ(buffer, corrupt);
      if (TestHasFailed()) {
        return;
      }
    }
  }
}

TEST(RequestDecoderTest, MethodTooLong) {
  const std::string kRequest = absl::StrCat(std::string(kDecodeBufferSize, 'X'),
                                            " / HTTP/1.1\r\n"
                                            "\r\n");
  RequestDecoder decoder;
  for (const auto extra_method : {"", "Y", "YY"}) {
    const auto request = absl::StrCat(extra_method, kRequest);
    for (auto partition : GenerateMultipleRequestPartitions(request)) {
      LOG(INFO) << "\n"
                << "----------------------------------------"
                << "----------------------------------------";
      StrictMock<MockRequestDecoderListener> rdl;
      CollapsingRequestDecoderListener crdl(rdl);
      InSequence s;
      ExpectError(rdl, "HTTP Method too long");

      const auto [status, buffer, remainder] =
          DecodePartitionedRequest(decoder, crdl, partition);
      EXPECT_EQ(status, EDecodeBufferStatus::kIllFormed);
      EXPECT_THAT(request, StartsWith(buffer));
      EXPECT_EQ(remainder, request);
      if (TestHasFailed()) {
        return;
      }
    }
  }
}

TEST(RequestDecoderTest, InvalidPathStart) {
  RequestDecoder decoder;
  for (const char c : AllCharsExcept([](char c) { return c == '/'; })) {
    LOG(INFO) << "\n"
              << "----------------------------------------"
              << "----------------------------------------";
    const auto corrupt = absl::StrCat(std::string(1, c),
                                      " HTTP/1.1\r\n"
                                      "\r\n");
    const auto request = absl::StrCat("GET ", corrupt);
    std::string buffer = request;
    StrictMock<MockRequestDecoderListener> rdl;
    CollapsingRequestDecoderListener crdl(rdl);
    ExpectCompleteText(rdl, EToken::kHttpMethod, "GET");
    ExpectError(rdl, "Invalid path start");

    const auto status = ResetAndDecodeFullBuffer(decoder, crdl, buffer);
    EXPECT_EQ(status, EDecodeBufferStatus::kIllFormed);
    EXPECT_EQ(buffer, corrupt);
    if (TestHasFailed()) {
      return;
    }
  }
}

TEST(RequestDecoderTest, InvalidEncodedPathChar) {
  RequestDecoder decoder;
  for (const auto corrupt_pct_char : GenerateInvalidPercentEncodedChar()) {
    LOG(INFO) << "\n"
              << "----------------------------------------"
              << "----------------------------------------";
    const auto corrupt = absl::StrCat(corrupt_pct_char,
                                      " HTTP/1.1\r\n"
                                      "\r\n");
    const auto request = absl::StrCat("GET /", corrupt);
    std::string buffer = request;
    MockRequestDecoderListener rdl(/*on_unexpected_call_stop_decoding=*/false);
    ExpectCompleteText(rdl, EToken::kHttpMethod, "GET");
    ExpectEvent(rdl, EEvent::kPathStart);
    ExpectError(rdl, "Invalid Percent-Encoded Character");

    const auto status = ResetAndDecodeFullBuffer(decoder, rdl, buffer);
    EXPECT_EQ(status, EDecodeBufferStatus::kIllFormed);
    EXPECT_EQ(buffer, corrupt);
    Mock::VerifyAndClearExpectations(&rdl);
    if (TestHasFailed()) {
      return;
    }
  }
}

TEST(RequestDecoderTest, InvalidPathChar) {
  RequestDecoder decoder;
  for (const char c : AllCharsExcept([](char c) {
         return IsPChar(c) || c == '/' || c == '?' || c == ' ';
       })) {
    for (const auto slash_at_end : {true, false}) {
      LOG(INFO) << "\n"
                << "----------------------------------------"
                << "----------------------------------------";
      const auto corrupt = absl::StrCat(std::string(1, c),
                                        "bar HTTP/1.1\r\n"
                                        "\r\n");
      const auto request =
          absl::StrCat("GET /foo", (slash_at_end ? "/" : ""), corrupt);
      std::string buffer = request;
      StrictMock<MockRequestDecoderListener> rdl;
      CollapsingRequestDecoderListener crdl(rdl);
      InSequence s;
      ExpectCompleteText(rdl, EToken::kHttpMethod, "GET");
      ExpectEvent(rdl, EEvent::kPathStart);
      ExpectCompleteText(rdl, EToken::kPathSegment, "foo");
      if (slash_at_end) {
        ExpectEvent(rdl, EEvent::kPathSeparator);
      }
      ExpectEvent(rdl, EEvent::kPathEnd);
      ExpectError(rdl, "Invalid request target end");

      const auto status = ResetAndDecodeFullBuffer(decoder, crdl, buffer);
      EXPECT_EQ(status, EDecodeBufferStatus::kIllFormed);
      EXPECT_EQ(buffer, corrupt);
      if (TestHasFailed()) {
        return;
      }
    }
  }
}

TEST(RequestDecoderTest, MissingParamName) {
  RequestDecoder decoder;
  const char kCorrupt[] = "=value HTTP/1.1\r\n\r\n";
  const auto request = absl::StrCat("GET /?", kCorrupt);
  std::string buffer = request;
  MockRequestDecoderListener rdl(/*on_unexpected_call_stop_decoding=*/false);
  ExpectCompleteText(rdl, EToken::kHttpMethod, "GET");
  ExpectEvent(rdl, EEvent::kPathStart);
  ExpectEvent(rdl, EEvent::kPathEndQueryStart);
  ExpectError(rdl, "Param name missing");

  const auto status = ResetAndDecodeFullBuffer(decoder, rdl, buffer);
  EXPECT_EQ(status, EDecodeBufferStatus::kIllFormed);
  EXPECT_EQ(buffer, kCorrupt);
}

TEST(RequestDecoderTest, InvalidParamNameChar) {
  RequestDecoder decoder;
  for (const auto corrupt_pct_char : GenerateInvalidPercentEncodedChar()) {
    LOG(INFO) << "\n"
              << "----------------------------------------"
              << "----------------------------------------";
    const auto corrupt = absl::StrCat(corrupt_pct_char,
                                      "=value HTTP/1.1\r\n"
                                      "\r\n");
    const auto request = absl::StrCat("GET /?", corrupt);
    std::string buffer = request;
    MockRequestDecoderListener rdl(/*on_unexpected_call_stop_decoding=*/false);
    ExpectCompleteText(rdl, EToken::kHttpMethod, "GET");
    ExpectEvent(rdl, EEvent::kPathStart);
    ExpectEvent(rdl, EEvent::kPathEndQueryStart);
    ExpectError(rdl, "Invalid Percent-Encoded Character");

    const auto status = ResetAndDecodeFullBuffer(decoder, rdl, buffer);
    EXPECT_EQ(status, EDecodeBufferStatus::kIllFormed);
    EXPECT_EQ(buffer, corrupt);
    Mock::VerifyAndClearExpectations(&rdl);
    if (TestHasFailed()) {
      return;
    }
  }
}

TEST(RequestDecoderTest, InvalidParamValueChar) {
  RequestDecoder decoder;
  for (const auto corrupt_pct_char : GenerateInvalidPercentEncodedChar()) {
    LOG(INFO) << "\n"
              << "----------------------------------------"
              << "----------------------------------------";
    const auto corrupt = absl::StrCat(corrupt_pct_char,
                                      " HTTP/1.1\r\n"
                                      "\r\n");
    const auto request = absl::StrCat("GET /?name=", corrupt);
    std::string buffer = request;
    MockRequestDecoderListener rdl(/*on_unexpected_call_stop_decoding=*/false);
    ExpectCompleteText(rdl, EToken::kHttpMethod, "GET");
    ExpectEvent(rdl, EEvent::kPathStart);
    ExpectEvent(rdl, EEvent::kPathEndQueryStart);
    ExpectCompleteText(rdl, EToken::kParamName, "name");
    ExpectError(rdl, "Invalid Percent-Encoded Character");

    const auto status = ResetAndDecodeFullBuffer(decoder, rdl, buffer);
    EXPECT_EQ(status, EDecodeBufferStatus::kIllFormed);
    EXPECT_EQ(buffer, corrupt);
    Mock::VerifyAndClearExpectations(&rdl);
    if (TestHasFailed()) {
      return;
    }
  }
}

TEST(RequestDecoderTest, InvalidRawQueryStringChar) {
  RequestDecoder decoder;
  for (const char c :
       AllCharsExcept([](char c) { return IsQueryChar(c) || c == ' '; })) {
    for (const auto has_query_text : {true, false}) {
      LOG(INFO) << "\n"
                << "----------------------------------------"
                << "----------------------------------------";
      LOG(INFO) << "has_query_text=" << has_query_text;
      const auto corrupt = absl::StrCat(std::string(1, c),
                                        " HTTP/1.1\r\n"
                                        "\r\n");
      const auto request =
          absl::StrCat("GET /?", (has_query_text ? "a/b?c=d" : ""), corrupt);
      std::string buffer = request;
      StrictMock<MockRequestDecoderListener> rdl;
      CollapsingRequestDecoderListener crdl(rdl);
      InSequence s;
      ExpectCompleteText(rdl, EToken::kHttpMethod, "GET");
      ExpectEvent(rdl, EEvent::kPathStart);
      ExpectQueryStart(rdl, /*skip_query_string_decoding=*/true);
      if (has_query_text) {
        ExpectPartialTextMatching(rdl, EPartialToken::kRawQueryString,
                                  "a/b?c=d");
      }
      ExpectError(rdl, "Invalid request target end");

      const auto status = ResetAndDecodeFullBuffer(decoder, crdl, buffer);
      EXPECT_EQ(status, EDecodeBufferStatus::kIllFormed);
      EXPECT_EQ(buffer, corrupt);
      if (TestHasFailed()) {
        return;
      }
    }
  }
}

TEST(RequestDecoderTest, UnsupportedHttpVersion) {
  for (const char* faux_version :
       {"HTTP/0.9", "HTTP/1.0", "HTTP/1.2", "http/1.1", "HTTP/11", " HTTP/1.1",
        "HTTP/1.1 "}) {
    const auto request_tail = absl::StrCat(faux_version, "\r\n\r\n");
    const auto request = absl::StrCat("GET / ", request_tail);
    RequestDecoder decoder;
    for (auto partition : GenerateMultipleRequestPartitions(request)) {
      LOG(INFO) << "\n"
                << "----------------------------------------"
                << "----------------------------------------";
      StrictMock<MockRequestDecoderListener> rdl;
      CollapsingRequestDecoderListener crdl(rdl);
      InSequence s;
      ExpectCompleteText(rdl, EToken::kHttpMethod, "GET");
      ExpectEvent(rdl, EEvent::kPathStart);
      ExpectEvent(rdl, EEvent::kPathEnd);
      ExpectError(rdl, "Unsupported HTTP version");

      const auto [status, buffer, remainder] =
          DecodePartitionedRequest(decoder, crdl, partition);
      EXPECT_EQ(status, EDecodeBufferStatus::kIllFormed);
      EXPECT_THAT(request_tail, StartsWith(buffer));
      EXPECT_EQ(remainder, request_tail);
      if (TestHasFailed()) {
        return;
      }
    }
  }
}

TEST(RequestDecoderTest, InvalidHeaderNameStart) {
  RequestDecoder decoder;
  for (const char c :
       AllCharsExcept([](char c) { return IsTokenChar(c) || c == '\r'; })) {
    LOG(INFO) << "\n"
              << "----------------------------------------"
              << "----------------------------------------";
    const auto corrupt = absl::StrCat(std::string(1, c),
                                      "Host: name\r\n"
                                      "\r\n");
    const auto request = absl::StrCat("GET / HTTP/1.1\r\n", corrupt);
    std::string buffer = request;
    StrictMock<MockRequestDecoderListener> rdl;
    CollapsingRequestDecoderListener crdl(rdl);
    InSequence s;
    ExpectCompleteText(rdl, EToken::kHttpMethod, "GET");
    ExpectEvent(rdl, EEvent::kPathStart);
    ExpectEvent(rdl, EEvent::kPathEnd);
    ExpectEvent(rdl, EEvent::kHttpVersion1_1);
    ExpectError(rdl, "Expected header name");

    const auto status = ResetAndDecodeFullBuffer(decoder, crdl, buffer);
    EXPECT_EQ(status, EDecodeBufferStatus::kIllFormed);
    EXPECT_EQ(buffer, corrupt);
    if (TestHasFailed()) {
      return;
    }
  }
}

TEST(RequestDecoderTest, InvalidHeaderNameValueSeparator) {
  RequestDecoder decoder;
  for (const char c :
       AllCharsExcept([](char c) { return IsTokenChar(c) || c == ':'; })) {
    LOG(INFO) << "\n"
              << "----------------------------------------"
              << "----------------------------------------";
    const auto corrupt = absl::StrCat(std::string(1, c),
                                      ": name\r\n"
                                      "\r\n");
    const auto request = absl::StrCat("GET / HTTP/1.1\r\nHost", corrupt);
    std::string buffer = request;
    StrictMock<MockRequestDecoderListener> rdl;
    CollapsingRequestDecoderListener crdl(rdl);
    InSequence s;
    ExpectCompleteText(rdl, EToken::kHttpMethod, "GET");
    ExpectEvent(rdl, EEvent::kPathStart);
    ExpectEvent(rdl, EEvent::kPathEnd);
    ExpectEvent(rdl, EEvent::kHttpVersion1_1);
    ExpectCompleteText(rdl, EToken::kHeaderName, "Host");
    ExpectError(rdl, "Expected colon after name");

    const auto status = ResetAndDecodeFullBuffer(decoder, crdl, buffer);
    EXPECT_EQ(status, EDecodeBufferStatus::kIllFormed);
    EXPECT_EQ(buffer, corrupt);
    if (TestHasFailed()) {
      return;
    }
  }
}

TEST(RequestDecoderTest, MissingHeaderValue) {
  const char kTail[] = "\r\n\r\n";
  RequestDecoder decoder;
  for (const std::string whitespace : {
           std::string(""),
           std::string(" "),
           std::string("\t"),
           std::string(kDecodeBufferSize, ' '),
           std::string(kDecodeBufferSize, '\t'),
           std::string(kDecodeBufferSize, ' ') +
               std::string(kDecodeBufferSize, '\t'),
       }) {
    LOG(INFO) << "\n"
              << "----------------------------------------"
              << "----------------------------------------";
    const auto request =
        absl::StrCat("GET / HTTP/1.1\r\nHost:", whitespace, kTail);
    std::string buffer = request;
    StrictMock<MockRequestDecoderListener> rdl;
    CollapsingRequestDecoderListener crdl(rdl);
    InSequence s;
    ExpectCompleteText(rdl, EToken::kHttpMethod, "GET");
    ExpectEvent(rdl, EEvent::kPathStart);
    ExpectEvent(rdl, EEvent::kPathEnd);
    ExpectEvent(rdl, EEvent::kHttpVersion1_1);
    ExpectCompleteText(rdl, EToken::kHeaderName, "Host");
    ExpectError(rdl, "Empty header value");

    const auto status = ResetAndDecodeFullBuffer(decoder, crdl, buffer);
    EXPECT_EQ(status, EDecodeBufferStatus::kIllFormed);
    EXPECT_EQ(buffer, kTail);
    if (TestHasFailed()) {
      return;
    }
  }
}

TEST(RequestDecoderTest, IncorrectHeaderLineEnd) {
  const char kTail[] = "\r\n\r\n";
  RequestDecoder decoder;
  for (const char c :
       AllCharsExcept([](char c) { return IsFieldContent(c) || c == '\r'; })) {
    LOG(INFO) << "\n"
              << "----------------------------------------"
              << "----------------------------------------";
    const auto corrupt = absl::StrCat(std::string(1, c), kTail);
    const auto request = absl::StrCat("GET / HTTP/1.1\r\nname:value", corrupt);
    std::string buffer = request;
    StrictMock<MockRequestDecoderListener> rdl;
    CollapsingRequestDecoderListener crdl(rdl);
    InSequence s;
    ExpectCompleteText(rdl, EToken::kHttpMethod, "GET");
    ExpectEvent(rdl, EEvent::kPathStart);
    ExpectEvent(rdl, EEvent::kPathEnd);
    ExpectEvent(rdl, EEvent::kHttpVersion1_1);
    ExpectCompleteText(rdl, EToken::kHeaderName, "name");
    ExpectCompleteText(rdl, EToken::kHeaderValue, "value");
    ExpectError(rdl, "Missing EOL after header");

    const auto status = ResetAndDecodeFullBuffer(decoder, crdl, buffer);
    EXPECT_EQ(status, EDecodeBufferStatus::kIllFormed);
    EXPECT_EQ(buffer, corrupt);
    if (TestHasFailed()) {
      return;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// Misc tests.

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

TEST(RequestDecoderTest, ResetRequired) {
  StrictMock<MockRequestDecoderListener> rdl;
  RequestDecoder decoder;
  const StringView original("GET ");
  auto buffer = original;
  EXPECT_EQ(decoder.DecodeBuffer(buffer, rdl, false),
            EDecodeBufferStatus::kInternalError);
  EXPECT_EQ(buffer, original);
}

// Test that the listener can call StopDecoding for essentially any event. We do
// this by measuring
TEST(RequestDecoderTest, StopDecodingFullRequest) {
  // Request with both short and oversize entities.
  const char kRequestHeader[] =
      "HEAD /1/23/01234567890123456789/%20some?a%20=b%20&c%20=&&d HTTP/1.1\r\n"
      "A: B\r\n"
      "cc: dd\r\n"
      "EEEEEEEEEEEEEEEEEEEE: FFFFFFFFFFFFFFFFFFFF\r\n"
      "\r\n";

  // Count the number of decoding events.
  const size_t num_events = [kRequestHeader]() {
    size_t num_events = 0;
    MockRequestDecoderListener rdl;
    EXPECT_CALL(rdl, OnEvent).WillRepeatedly(InvokeWithoutArgs([&num_events]() {
      ++num_events;
    }));
    EXPECT_CALL(rdl, OnCompleteText)
        .WillRepeatedly(InvokeWithoutArgs([&num_events]() { ++num_events; }));
    EXPECT_CALL(rdl, OnPartialText)
        .WillRepeatedly(InvokeWithoutArgs([&num_events]() { ++num_events; }));
    EXPECT_CALL(rdl, OnError).WillRepeatedly(InvokeWithoutArgs([&num_events]() {
      ++num_events;
    }));
    RequestDecoder decoder;
    decoder.Reset();
    std::string buffer = kRequestHeader;
    EXPECT_EQ(ResetAndDecodeFullBuffer(decoder, rdl, buffer,
                                       kMinimumDecodeBufferSize),
              EDecodeBufferStatus::kComplete);
    EXPECT_EQ(buffer, "");
    return num_events;
  }();
  EXPECT_GT(num_events, 30);
  if (TestHasFailed()) {
    return;
  }

  // Now call stop decoding at each of the listener calls.
  RequestDecoder decoder;
  for (int stop_on = 1; stop_on <= num_events; ++stop_on) {
    LOG(INFO) << "\n"
              << "----------------------------------------"
              << "----------------------------------------"
              << "\n            stop_on=" << stop_on << "\n";
    int remaining_events = stop_on;
    MockRequestDecoderListener rdl;
    auto ready_to_call_stop_decoding = [&remaining_events]() -> bool {
      EXPECT_GT(remaining_events, 0);
      return --remaining_events == 0;
    };
    EXPECT_CALL(rdl, OnEvent)
        .WillRepeatedly(
            [&ready_to_call_stop_decoding](const OnEventData& data) {
              if (ready_to_call_stop_decoding()) {
                data.StopDecoding();
              }
            });
    EXPECT_CALL(rdl, OnCompleteText)
        .WillRepeatedly(
            [&ready_to_call_stop_decoding](const OnCompleteTextData& data) {
              if (ready_to_call_stop_decoding()) {
                data.StopDecoding();
              }
            });
    EXPECT_CALL(rdl, OnPartialText)
        .WillRepeatedly(
            [&ready_to_call_stop_decoding](const OnPartialTextData& data) {
              if (ready_to_call_stop_decoding()) {
                data.StopDecoding();
              }
            });
    EXPECT_CALL(rdl, OnError)
        .WillRepeatedly(
            [&ready_to_call_stop_decoding](const OnErrorData& data) {
              if (ready_to_call_stop_decoding()) {
                data.StopDecoding();
              }
            });
    std::string buffer = kRequestHeader;
    const auto status = ResetAndDecodeFullBuffer(decoder, rdl, buffer,
                                                 kMinimumDecodeBufferSize);
    EXPECT_EQ(remaining_events, 0);
    if (stop_on == num_events) {
      EXPECT_EQ(status, EDecodeBufferStatus::kComplete);
      EXPECT_EQ(buffer, "");
    } else {
      EXPECT_EQ(status, EDecodeBufferStatus::kInternalError);
    }
    if (TestHasFailed()) {
      return;
    }
  }
}

}  // namespace
}  // namespace test
}  // namespace http1
}  // namespace mcunet
