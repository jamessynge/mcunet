#ifndef MCUNET_EXTRAS_TEST_TOOLS_MOCK_REQUEST_DECODER_LISTENER_H_
#define MCUNET_EXTRAS_TEST_TOOLS_MOCK_REQUEST_DECODER_LISTENER_H_

#include "gmock/gmock.h"
#include "http1/request_decoder.h"

namespace mcunet {
namespace http1 {
namespace test {

class MockRequestDecoderListener : public RequestDecoderListener {
 public:
  MOCK_METHOD(void, OnEvent, (EEvent event), (override));

  MOCK_METHOD(void, OnCompleteText, (EToken token, mcucore::StringView text),
              (override));

  MOCK_METHOD(void, OnPartialText,
              (EPartialToken token, EPartialTokenPosition position,
               mcucore::StringView text),
              (override));

  MOCK_METHOD(void, OnEnd, (), (override));

  MOCK_METHOD(void, OnError, (mcucore::ProgmemString msg), (override));
};

}  // namespace test
}  // namespace http1
}  // namespace mcunet

#endif  // MCUNET_EXTRAS_TEST_TOOLS_MOCK_REQUEST_DECODER_LISTENER_H_
