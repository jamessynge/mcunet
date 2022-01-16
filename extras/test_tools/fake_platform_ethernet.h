#ifndef MCUNET_EXTRAS_TEST_TOOLS_FAKE_PLATFORM_ETHERNET_H_
#define MCUNET_EXTRAS_TEST_TOOLS_FAKE_PLATFORM_ETHERNET_H_

// Goal: provide the ability to test ServerSocket, etc.
//
// TODO(jamessynge): Develop this idea.
//
// Author: james.synge@gmail.com

#include "platform_ethernet.h"

namespace mcunet {
namespace test {

class FakePlatformEthernet : public PlatformEthernetInterface {
 public:
 private:
  struct FakeConnection {};
};

}  // namespace test
}  // namespace mcunet

#endif  // MCUNET_EXTRAS_TEST_TOOLS_FAKE_PLATFORM_ETHERNET_H_
