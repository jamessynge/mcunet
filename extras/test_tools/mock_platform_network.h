#ifndef MCUNET_EXTRAS_TEST_TOOLS_MOCK_PLATFORM_NETWORK_H_
#define MCUNET_EXTRAS_TEST_TOOLS_MOCK_PLATFORM_NETWORK_H_

#include "gmock/gmock.h"
#include "platform_network.h"
#include "platform_network_interface.h"

namespace mcunet {
namespace test {

class MockPlatformNetwork : public PlatformNetworkInterface {
 public:
#ifdef MCUNET_PNAPI_METHOD
#error "MCUNET_PNAPI_METHOD should not be defined!!"
#endif

#define MCUNET_PNAPI_METHOD(TYPE, NAME, ARGS) MOCK_METHOD(TYPE, NAME, ARGS)
#include "platform_network_api.cc.inc"  // IWYU pragma: export
#undef MCUNET_PNAPI_METHOD
};

}  // namespace test
}  // namespace mcunet

#endif  // MCUNET_EXTRAS_TEST_TOOLS_MOCK_PLATFORM_NETWORK_H_
