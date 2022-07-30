#include "http1/request_decoder.h"

#include <algorithm>
#include <cctype>
#include <limits>
#include <string>
#include <string_view>
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
#include "mcucore/extras/test_tools/progmem_string_utils.h"
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
using ::mcunet::http1::mcunet_http1_internal::IsFieldContent;
using ::mcunet::http1::mcunet_http1_internal::IsPChar;
using ::mcunet::http1::mcunet_http1_internal::IsQueryChar;
using ::mcunet::http1::mcunet_http1_internal::IsTChar;
using ::mcunet::test::AllCharsExcept;
using ::mcunet::test::AppendRemainder;
using ::mcunet::test::GenerateMultipleRequestPartitions;
using ::mcunet::test::SplitEveryN;
using ::testing::_;
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
        return;
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
      InSequence s;
      EXPECT_CALL(rdl, OnCompleteText(EToken::kHttpMethod, Eq("HEAD")));
      EXPECT_CALL(rdl, OnEvent(EEvent::kPathStart));
      EXPECT_CALL(rdl, OnCompleteText(EToken::kPathSegment, Eq("setup")));
      EXPECT_CALL(rdl, OnEvent(EEvent::kPathSeparator));
      EXPECT_CALL(rdl, OnEvent(EEvent::kPathAndUrlEnd));
      EXPECT_CALL(rdl, OnEvent(EEvent::kHttpVersion1_1));
      EXPECT_CALL(rdl, OnCompleteText(EToken::kHeaderName, Eq("Host")));
      EXPECT_CALL(rdl, OnCompleteText(EToken::kHeaderValue, Eq("example.com")));
      EXPECT_CALL(rdl, OnEnd);

      const auto [status, buffer, remainder] =
          DecodePartitionedRequest(decoder, &rdl, partition);
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
      InSequence s;
      EXPECT_CALL(rdl, OnCompleteText(EToken::kHttpMethod, Eq("PUT")));
      EXPECT_CALL(rdl, OnEvent(EEvent::kPathStart));
      EXPECT_CALL(rdl, OnEvent(EEvent::kPathAndUrlEnd));
      EXPECT_CALL(rdl, OnEvent(EEvent::kHttpVersion1_1));
      EXPECT_CALL(rdl, OnCompleteText(EToken::kHeaderName, Eq("name")));
      EXPECT_CALL(rdl, OnCompleteText(EToken::kHeaderValue, Eq("some.text")));
      EXPECT_CALL(rdl, OnCompleteText(EToken::kHeaderName, Eq("Name")));
      EXPECT_CALL(rdl, OnCompleteText(EToken::kHeaderValue,
                                      Eq("text:with/interesting")));
      EXPECT_CALL(rdl, OnCompleteText(EToken::kHeaderName, Eq("NAME")));
      EXPECT_CALL(rdl, OnCompleteText(EToken::kHeaderValue,
                                      Eq("interesting\tcharacters")));
      EXPECT_CALL(rdl, OnCompleteText(EToken::kHeaderName, Eq("N-A-M-E")));
      EXPECT_CALL(rdl,
                  OnCompleteText(EToken::kHeaderValue, Eq("characters&in?it")));
      EXPECT_CALL(rdl, OnEnd);

      const auto [status, buffer, remainder] =
          DecodePartitionedRequest(decoder, &rdl, partition);
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

void AddPartialTextExpectations(MockRequestDecoderListener& rdl,
                                EPartialToken token, std::string& accumulator) {
  InSequence s;
  EXPECT_CALL(rdl, OnPartialText(token, EPartialTokenPosition::kFirst, _))
      .WillOnce([&accumulator](EPartialToken token,
                               EPartialTokenPosition position,
                               mcucore::StringView text) {
        accumulator = std::string(text.data(), text.size());
      });
  EXPECT_CALL(rdl, OnPartialText(token, EPartialTokenPosition::kMiddle, _))
      .WillRepeatedly([&accumulator](EPartialToken token,
                                     EPartialTokenPosition position,
                                     mcucore::StringView text) {
        accumulator += std::string(text.data(), text.size());
      });
  EXPECT_CALL(rdl, OnPartialText(token, EPartialTokenPosition::kLast, _))
      .WillOnce([&accumulator](EPartialToken token,
                               EPartialTokenPosition position,
                               mcucore::StringView text) {
        accumulator += std::string(text.data(), text.size());
      });
}

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
                       (query_after_path ? "?" : ""), kRestOfRequest);
      for (auto partition : GenerateMultipleRequestPartitions(request)) {
        LOG(INFO) << "\n"
                  << "----------------------------------------"
                  << "----------------------------------------";
        std::string segment1, segment2;
        StrictMock<MockRequestDecoderListener> rdl;
        InSequence s;
        EXPECT_CALL(rdl, OnCompleteText(EToken::kHttpMethod, Eq("GET")));
        EXPECT_CALL(rdl, OnEvent(EEvent::kPathStart));
        AddPartialTextExpectations(rdl, EPartialToken::kPathSegment, segment1);
        EXPECT_CALL(rdl, OnEvent(EEvent::kPathSeparator));
        AddPartialTextExpectations(rdl, EPartialToken::kPathSegment, segment2);
        if (slash_at_end) {
          EXPECT_CALL(rdl, OnEvent(EEvent::kPathSeparator));
        }
        if (query_after_path) {
          EXPECT_CALL(rdl, OnEvent(EEvent::kPathEndQueryStart));
          EXPECT_CALL(rdl, OnEvent(EEvent::kQueryAndUrlEnd));
        } else {
          EXPECT_CALL(rdl, OnEvent(EEvent::kPathAndUrlEnd));
        }
        EXPECT_CALL(rdl, OnEvent(EEvent::kHttpVersion1_1));
        EXPECT_CALL(rdl, OnEnd);

        const auto [status, buffer, remainder] =
            DecodePartitionedRequest(decoder, &rdl, partition);
        EXPECT_EQ(status, EDecodeBufferStatus::kComplete);
        EXPECT_EQ(buffer, "");
        EXPECT_EQ(remainder, "");
        EXPECT_EQ(segment1, kSegment1);
        EXPECT_EQ(segment2, kSegment2);
        if (TestHasFailed()) {
          return;
        }
      }
    }
  }
}

TEST(RequestDecoderTest, LongQueryString) {
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
        std::string assembled_query_string;
        StrictMock<MockRequestDecoderListener> rdl;
        InSequence s;
        EXPECT_CALL(rdl, OnCompleteText(EToken::kHttpMethod, Eq("GET")));
        EXPECT_CALL(rdl, OnEvent(EEvent::kPathStart));
        if (path.size() > 1) {
          EXPECT_CALL(rdl, OnCompleteText(EToken::kPathSegment, Eq("p")));
          if (path.size() > 2) {
            EXPECT_CALL(rdl, OnEvent(EEvent::kPathSeparator));
          }
        }
        EXPECT_CALL(rdl, OnEvent(EEvent::kPathEndQueryStart));
        AddPartialTextExpectations(rdl, EPartialToken::kQueryString,
                                   assembled_query_string);
        EXPECT_CALL(rdl, OnEvent(EEvent::kHttpVersion1_1));
        EXPECT_CALL(rdl, OnEnd);

        const auto [status, buffer, remainder] =
            DecodePartitionedRequest(decoder, &rdl, partition);
        EXPECT_EQ(status, EDecodeBufferStatus::kComplete);
        EXPECT_EQ(buffer, "");
        EXPECT_EQ(remainder, "");
        EXPECT_EQ(assembled_query_string, query_string);
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
    std::string name1, value1, name2, value2;
    StrictMock<MockRequestDecoderListener> rdl;
    InSequence s;
    EXPECT_CALL(rdl, OnCompleteText(EToken::kHttpMethod, Eq("GET")));
    EXPECT_CALL(rdl, OnEvent(EEvent::kPathStart));
    EXPECT_CALL(rdl, OnEvent(EEvent::kPathAndUrlEnd));
    EXPECT_CALL(rdl, OnEvent(EEvent::kHttpVersion1_1));
    AddPartialTextExpectations(rdl, EPartialToken::kHeaderName, name1);
    AddPartialTextExpectations(rdl, EPartialToken::kHeaderValue, value1);
    AddPartialTextExpectations(rdl, EPartialToken::kHeaderName, name2);
    AddPartialTextExpectations(rdl, EPartialToken::kHeaderValue, value2);
    EXPECT_CALL(rdl, OnEnd);

    const auto [status, buffer, remainder] =
        DecodePartitionedRequest(decoder, &rdl, partition);
    EXPECT_EQ(status, EDecodeBufferStatus::kComplete);
    EXPECT_EQ(buffer, "");
    EXPECT_EQ(remainder, "");
    EXPECT_EQ(name1, kName1);
    EXPECT_EQ(value1, kValue1);
    EXPECT_EQ(name2, kName2);
    EXPECT_EQ(value2, kValue2);
    if (TestHasFailed()) {
      return;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// Tests of the ability to detect and reject malformed requests.

TEST(RequestDecoderTest, InvalidMethodStart) {
  RequestDecoder decoder;
  for (const char c : AllCharsExcept(std::isupper)) {
    LOG(INFO) << "\n"
              << "----------------------------------------"
              << "----------------------------------------";
    const auto request = std::string(1, c) +
                         "GET / HTTP/1.1\r\n"
                         "\r\n";
    std::string buffer = request;
    StrictMock<MockRequestDecoderListener> rdl;
    EXPECT_CALL(rdl, OnError(Eq("Invalid HTTP method start")));

    const auto status = ResetAndDecodeFullBuffer(decoder, &rdl, buffer);
    EXPECT_EQ(status, EDecodeBufferStatus::kIllFormed);
    EXPECT_EQ(buffer, request);
    if (TestHasFailed()) {
      return;
    }
  }
}

TEST(RequestDecoderTest, InvalidMethodEnd) {
  RequestDecoder decoder;
  for (const char c :
       AllCharsExcept([](char c) { return std::isupper(c) || c == ' '; })) {
    for (const auto space_before_path : {true, false}) {
      LOG(INFO) << "\n"
                << "----------------------------------------"
                << "----------------------------------------";
      const auto request =
          absl::StrCat("GET", std::string(1, c), (space_before_path ? " " : ""),
                       "/ HTTP/1.1\r\n"
                       "\r\n");
      std::string buffer = request;
      StrictMock<MockRequestDecoderListener> rdl;
      EXPECT_CALL(rdl, OnError(Eq("Invalid HTTP method end")));

      const auto status = ResetAndDecodeFullBuffer(decoder, &rdl, buffer);
      EXPECT_EQ(status, EDecodeBufferStatus::kIllFormed);
      EXPECT_EQ(buffer, request);
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
      InSequence s;
      EXPECT_CALL(rdl, OnError(Eq("HTTP Method too long")));

      const auto [status, buffer, remainder] =
          DecodePartitionedRequest(decoder, &rdl, partition);
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
    EXPECT_CALL(rdl, OnCompleteText(EToken::kHttpMethod, Eq("GET")));
    EXPECT_CALL(rdl, OnError(Eq("Invalid path start")));

    const auto status = ResetAndDecodeFullBuffer(decoder, &rdl, buffer);
    EXPECT_EQ(status, EDecodeBufferStatus::kIllFormed);
    EXPECT_EQ(buffer, corrupt);
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
      EXPECT_CALL(rdl, OnCompleteText(EToken::kHttpMethod, Eq("GET")));
      EXPECT_CALL(rdl, OnEvent(EEvent::kPathStart));
      EXPECT_CALL(rdl, OnCompleteText(EToken::kPathSegment, Eq("foo")));
      if (slash_at_end) {
        EXPECT_CALL(rdl, OnEvent(EEvent::kPathSeparator));
      }
      EXPECT_CALL(rdl, OnEvent(EEvent::kPathAndUrlEnd));
      EXPECT_CALL(rdl, OnError(Eq("Invalid request target end")));

      const auto status = ResetAndDecodeFullBuffer(decoder, &rdl, buffer);
      EXPECT_EQ(status, EDecodeBufferStatus::kIllFormed);
      EXPECT_EQ(buffer, corrupt);
      if (TestHasFailed()) {
        return;
      }
    }
  }
}

TEST(RequestDecoderTest, InvalidQueryChar) {
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
      EXPECT_CALL(rdl, OnCompleteText(EToken::kHttpMethod, Eq("GET")));
      EXPECT_CALL(rdl, OnEvent(EEvent::kPathStart));
      EXPECT_CALL(rdl, OnEvent(EEvent::kPathEndQueryStart));
      if (has_query_text) {
        EXPECT_CALL(
            rdl, OnPartialText(EPartialToken::kQueryString,
                               EPartialTokenPosition::kFirst, Eq("a/b?c=d")));
        EXPECT_CALL(rdl, OnPartialText(EPartialToken::kQueryString,
                                       EPartialTokenPosition::kLast, Eq("")));
      } else {
        EXPECT_CALL(rdl, OnEvent(EEvent::kQueryAndUrlEnd));
      }
      EXPECT_CALL(rdl, OnError(Eq("Invalid request target end")));

      const auto status = ResetAndDecodeFullBuffer(decoder, &rdl, buffer);
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
      InSequence s;
      EXPECT_CALL(rdl, OnCompleteText(EToken::kHttpMethod, Eq("GET")));
      EXPECT_CALL(rdl, OnEvent(EEvent::kPathStart));
      EXPECT_CALL(rdl, OnEvent(EEvent::kPathAndUrlEnd));
      EXPECT_CALL(rdl, OnError(Eq("Unsupported HTTP version")));

      const auto [status, buffer, remainder] =
          DecodePartitionedRequest(decoder, &rdl, partition);
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
       AllCharsExcept([](char c) { return IsTChar(c) || c == '\r'; })) {
    LOG(INFO) << "\n"
              << "----------------------------------------"
              << "----------------------------------------";
    const auto corrupt = absl::StrCat(std::string(1, c),
                                      "Host: name\r\n"
                                      "\r\n");
    const auto request = absl::StrCat("GET / HTTP/1.1\r\n", corrupt);
    std::string buffer = request;
    StrictMock<MockRequestDecoderListener> rdl;
    EXPECT_CALL(rdl, OnCompleteText(EToken::kHttpMethod, Eq("GET")));
    EXPECT_CALL(rdl, OnEvent(EEvent::kPathStart));
    EXPECT_CALL(rdl, OnEvent(EEvent::kPathAndUrlEnd));
    EXPECT_CALL(rdl, OnEvent(EEvent::kHttpVersion1_1));
    EXPECT_CALL(rdl, OnError(Eq("Expected header name")));

    const auto status = ResetAndDecodeFullBuffer(decoder, &rdl, buffer);
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
       AllCharsExcept([](char c) { return IsTChar(c) || c == ':'; })) {
    LOG(INFO) << "\n"
              << "----------------------------------------"
              << "----------------------------------------";
    const auto corrupt = absl::StrCat(std::string(1, c),
                                      ": name\r\n"
                                      "\r\n");
    const auto request = absl::StrCat("GET / HTTP/1.1\r\nHost", corrupt);
    std::string buffer = request;
    StrictMock<MockRequestDecoderListener> rdl;
    EXPECT_CALL(rdl, OnCompleteText(EToken::kHttpMethod, Eq("GET")));
    EXPECT_CALL(rdl, OnEvent(EEvent::kPathStart));
    EXPECT_CALL(rdl, OnEvent(EEvent::kPathAndUrlEnd));
    EXPECT_CALL(rdl, OnEvent(EEvent::kHttpVersion1_1));
    EXPECT_CALL(rdl, OnCompleteText(EToken::kHeaderName, Eq("Host")));
    EXPECT_CALL(rdl, OnError(Eq("Expected colon after name")));

    const auto status = ResetAndDecodeFullBuffer(decoder, &rdl, buffer);
    EXPECT_EQ(status, EDecodeBufferStatus::kIllFormed);
    EXPECT_EQ(buffer, corrupt);
    if (TestHasFailed()) {
      return;
    }
  }
}

TEST(RequestDecoderTest, EmptyHeaderValue) {
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
    EXPECT_CALL(rdl, OnCompleteText(EToken::kHttpMethod, Eq("GET")));
    EXPECT_CALL(rdl, OnEvent(EEvent::kPathStart));
    EXPECT_CALL(rdl, OnEvent(EEvent::kPathAndUrlEnd));
    EXPECT_CALL(rdl, OnEvent(EEvent::kHttpVersion1_1));
    EXPECT_CALL(rdl, OnCompleteText(EToken::kHeaderName, Eq("Host")));
    EXPECT_CALL(rdl, OnError(Eq("Empty header value")));

    const auto status = ResetAndDecodeFullBuffer(decoder, &rdl, buffer);
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
    EXPECT_CALL(rdl, OnCompleteText(EToken::kHttpMethod, Eq("GET")));
    EXPECT_CALL(rdl, OnEvent(EEvent::kPathStart));
    EXPECT_CALL(rdl, OnEvent(EEvent::kPathAndUrlEnd));
    EXPECT_CALL(rdl, OnEvent(EEvent::kHttpVersion1_1));
    EXPECT_CALL(rdl, OnCompleteText(EToken::kHeaderName, Eq("name")));
    EXPECT_CALL(rdl, OnCompleteText(EToken::kHeaderValue, Eq("value")));
    EXPECT_CALL(rdl, OnError(Eq("Missing EOL after header")));

    const auto status = ResetAndDecodeFullBuffer(decoder, &rdl, buffer);
    EXPECT_EQ(status, EDecodeBufferStatus::kIllFormed);
    EXPECT_EQ(buffer, corrupt);
    if (TestHasFailed()) {
      return;
    }
  }
}

}  // namespace
}  // namespace test
}  // namespace http1
}  // namespace mcunet
