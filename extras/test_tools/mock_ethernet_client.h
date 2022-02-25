#ifndef MCUNET_EXTRAS_TEST_TOOLS_MOCK_ETHERNET_CLIENT_H_
#define MCUNET_EXTRAS_TEST_TOOLS_MOCK_ETHERNET_CLIENT_H_

#include <stdint.h>

#include <cstddef>

#include "extras/host/arduino/ip_address.h"
#include "extras/host/ethernet5500/ethernet_client.h"
#include "extras/test_tools/mock_client.h"
#include "gmock/gmock.h"

// Even though EthernetClient is in the root namespace, I've chosen to place
// MockEthernetClient into mcunet::test.
namespace mcunet {
namespace test {

class MockEthernetClient : public EthernetClient, public MockClient {
 public:
  MOCK_METHOD(uint8_t, status, (), (override));
  MOCK_METHOD(uint8_t, getSocketNumber, (), (const, override));
};

}  // namespace test
}  // namespace mcunet

#endif  // MCUNET_EXTRAS_TEST_TOOLS_MOCK_ETHERNET_CLIENT_H_
