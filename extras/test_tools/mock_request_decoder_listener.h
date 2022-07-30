#ifndef MCUNET_EXTRAS_TEST_TOOLS_MOCK_REQUEST_DECODER_LISTENER_H_
#define MCUNET_EXTRAS_TEST_TOOLS_MOCK_REQUEST_DECODER_LISTENER_H_

#include <string>
#include <string_view>

#include "gmock/gmock.h"
#include "http1/request_decoder.h"

namespace mcunet {
namespace http1 {
namespace test {

class MockRequestDecoderListener : public RequestDecoderListener {
 public:
  MOCK_METHOD(void, OnEvent, (const OnEventData& data), (override));

  MOCK_METHOD(void, OnCompleteText, (const OnCompleteTextData& data),
              (override));

  MOCK_METHOD(void, OnPartialText, (const OnPartialTextData& data), (override));

  MOCK_METHOD(void, OnError, (const OnErrorData& data), (override));
};

void ExpectEvent(MockRequestDecoderListener& rdl, EEvent event);
void ExpectCompleteText(MockRequestDecoderListener& rdl, EToken token,
                        std::string_view text);
void ExpectPartialTextMatching(MockRequestDecoderListener& rdl,
                               EPartialToken token, std::string text);
void ExpectError(MockRequestDecoderListener& rdl, std::string message);

}  // namespace test
}  // namespace http1
}  // namespace mcunet

#endif  // MCUNET_EXTRAS_TEST_TOOLS_MOCK_REQUEST_DECODER_LISTENER_H_
