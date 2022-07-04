#ifndef MCUNET_EXTRAS_TEST_TOOLS_FAKE_PLATFORM_NETWORK_H_
#define MCUNET_EXTRAS_TEST_TOOLS_FAKE_PLATFORM_NETWORK_H_

// Goal: provide the ability to test ServerSocket, etc.
//
// TODO(jamessynge): Develop this idea.
//
// Author: james.synge@gmail.com

#include "platform_network.h"

namespace mcunet {
namespace test {

class FakePlatformNetwork : public PlatformNetworkInterface {
 public:
  FakePlatformNetwork() {}
  ~FakePlatformNetwork() override {}

#ifdef MCUNET_PNAPI_METHOD
#error "MCUNET_PNAPI_METHOD should not be defined!!"
#endif

#define MCUNET_PNAPI_METHOD(TYPE, NAME, ARGS) virtual TYPE NAME ARGS = 0
#include "platform_network_api.cc.inc"
#undef MCUNET_PNAPI_METHOD
};

}  // namespace test
}  // namespace mcunet

#endif  // MCUNET_EXTRAS_TEST_TOOLS_FAKE_PLATFORM_NETWORK_H_
