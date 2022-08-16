#include "extras/host/ethernet5500/ethernet_client.h"

#include "gtest/gtest.h"

namespace mcunet_host {
namespace test {
namespace {

TEST(EthernetClientTest, NoFixtureTest) {
  EthernetClient a(1);
  a = EthernetClient(2);
}

}  // namespace
}  // namespace test
}  // namespace mcunet_host
