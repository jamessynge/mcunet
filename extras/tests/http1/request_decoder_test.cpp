#include "http1/request_decoder.h"

#include <algorithm>
#include <string>
#include <tuple>
#include <vector>

// TODO(jamessynge): Trim down the includes after writing tests.
#include <McuCore.h>

#include "extras/test_tools/mock_request_decoder_listener.h"
#include "extras/test_tools/string_utils.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
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

  while (true) {
    // We deliberately copy into another string. This can allow msan or asan to
    // detect when we've read too far.
    const size_t initial_size = std::min(max_decode_buffer_size, buffer.size());
    std::string copy = buffer.substr(0, initial_size);
    mcucore::StringView view(copy.data(), initial_size);

    const bool buffer_is_full = view.size() >= max_decode_buffer_size;
    auto status = decoder.DecodeBuffer(view, buffer_is_full);

    // Make sure that the decoder only removed the prefix of the view.
    EXPECT_GE(initial_size, view.size());
    const size_t removed_size = initial_size - view.size();
    EXPECT_EQ(copy.data() + removed_size, view.data());

    // Make sure that the decoder didn't modify the passed in buffer.
    EXPECT_EQ(buffer.substr(0, initial_size), copy.substr(0, initial_size));

    // Remove the decoded prefix of buffer.
    buffer.erase(0, initial_size - view.size());
    if (buffer.empty() || status != EDecodeBufferStatus::kDecodingInProgress) {
      return status;
    }
  }
}

EDecodeBufferStatus ResetAndDecodeFullBuffer(
    RequestDecoder& decoder, RequestDecoderListener* listener,
    std::string& buffer,
    const size_t max_decode_buffer_size = kDecodeBufferSize) {
  decoder.Reset();
  if (listener != nullptr) {
    decoder.SetListener(*listener);
  }
  return DecodeBuffer(decoder, buffer, true, max_decode_buffer_size);
}

// Apply the decoder to decoding the provided partition of a request. Returns
// the final decode status, the remainder of the last buffer passed in, and
// all the remaining undecoded text.
std::tuple<EDecodeBufferStatus, std::string, std::string>
DecodePartitionedRequest(
    RequestDecoder& decoder, RequestDecoderListener* listener,
    const std::vector<std::string>& partition,
    const size_t max_decode_buffer_size = kDecodeBufferSize) {
  CHECK_NE(partition.size(), 0);
  CHECK_GT(max_decode_buffer_size, 0);
  CHECK_LE(max_decode_buffer_size, mcucore::StringView::kMaxSize);
  decoder.Reset();
  if (listener != nullptr) {
    decoder.SetListener(*listener);
  }
  std::string buffer;
  for (int ndx = 0; ndx < partition.size(); ++ndx) {
    const bool at_end = (ndx + 1) == partition.size();
    buffer += partition[ndx];
    auto status = DecodeBuffer(decoder, buffer, at_end, max_decode_buffer_size);
    if (status >= EDecodeBufferStatus::kComplete) {
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
  StrictMock<MockRequestDecoderListener> listener;
  RequestDecoder decoder;
  decoder.SetListener(listener);
}

TEST(RequestDecoderTest, ResetRequired) {
  StrictMock<MockRequestDecoderListener> listener;
  RequestDecoder decoder;
  decoder.SetListener(listener);
  const StringView original("GET ");
  auto buffer = original;
  EXPECT_EQ(decoder.DecodeBuffer(buffer, false),
            EDecodeBufferStatus::kInternalError);
  EXPECT_EQ(buffer, original);
}

TEST(RequestDecoderTest, SmallestHomePageRequest) {
  RequestDecoder decoder;

  const std::string full_request(
      "GET / HTTP/1.1\r\n"
      "\r\n");

  for (auto partition : GenerateMultipleRequestPartitions(full_request)) {
    LOG(INFO) << "\n"
              << "----------------------------------------"
              << "----------------------------------------";
    StrictMock<MockRequestDecoderListener> listener;
    EXPECT_CALL(listener, OnCompleteText(EToken::kHttpMethod, Eq("GET")));
    EXPECT_CALL(listener, OnEvent(EEvent::kPathStart));
    EXPECT_CALL(listener, OnEvent(EEvent::kPathAndUrlEnd));
    EXPECT_CALL(listener, OnEvent(EEvent::kHttpVersion1_1));
    EXPECT_CALL(listener, OnEnd);

    const auto [status, buffer, remainder] =
        DecodePartitionedRequest(decoder, &listener, partition);
    EXPECT_EQ(status, EDecodeBufferStatus::kComplete);

    // const EHttpStatusCode status = std::get<0>(result);
    // const std::string buffer = std::get<1>(result);
    // const std::string remainder = std::get<2>(result);

    // EXPECT_EQ(status, EHttpStatusCode::kHttpOk);
    // EXPECT_THAT(buffer, IsEmpty());
    // EXPECT_THAT(remainder, IsEmpty());
    // EXPECT_EQ(alpaca_request.http_method, EHttpMethod::GET);

    // EXPECT_EQ(alpaca_request.api_group, EApiGroup::kServerStatus);
    // EXPECT_EQ(alpaca_request.api, EAlpacaApi::kServerStatus);
    // EXPECT_FALSE(alpaca_request.have_client_id);
    // EXPECT_FALSE(alpaca_request.have_client_transaction_id);

    if (TestHasFailed()) {
      break;
    }
  }
}

// TEST(NoFixture_RequestDecoderTest, NoFixtureTest) {
//   // TODO(jamessynge): Describe if not really obvious.
//   EXPECT_EQ(1, 1);
// }

// class RequestDecoderTest : public testing::Test {
//  protected:
//   RequestDecoderTest() {}
//   void SetUp() override {}
// };

// TEST_F(RequestDecoderTest, FixturedTest) {
//   // TODO(jamessynge): Describe if not really obvious.
//   EXPECT_EQ(1, 1);
// }

}  // namespace
}  // namespace test
}  // namespace http1
}  // namespace mcunet
