#ifndef MCUNET_EXTRAS_TEST_TOOLS_MOCK_ETHERNET_CLIENT_H_
#define MCUNET_EXTRAS_TEST_TOOLS_MOCK_ETHERNET_CLIENT_H_

// Mock class for EthernetClient. While EthernetClient is in the root namespace,
// I've chosen to place the mock class into a test namespace so that it is
// really clear that it is for use in tests; in some cases this avoids the need
// for a using declaration.
//
// The only types used here are those of the class(es) being mocked and those
// used in the methods being mocked. Even though IWYU would call for more
// headers to be included, I've chosen here to limit the includes in headers
// like this to gmock.h and the header(s) of the class(es) being mocked.

#include "extras/host/ethernet5500/ethernet_client.h"
#include "gmock/gmock.h"

namespace mcunet {
namespace test {

class MockEthernetClient : public EthernetClient {
 public:
  // Print methods:
  MOCK_METHOD(size_t, write, (uint8_t), (override));
  MOCK_METHOD(size_t, write, (const uint8_t *, size_t), (override));
  MOCK_METHOD(void, flush, (), (override));

  // Stream methods:
  MOCK_METHOD(int, available, (), (override));
  MOCK_METHOD(int, read, (), (override));
  MOCK_METHOD(int, peek, (), (override));

  // Client methods:
  MOCK_METHOD(int, connect, (class IPAddress, uint16_t), (override));
  MOCK_METHOD(int, connect, (const char *, uint16_t), (override));
  MOCK_METHOD(int, read, (uint8_t *, size_t), (override));
  MOCK_METHOD(void, stop, (), (override));
  MOCK_METHOD(uint8_t, connected, (), (override));

  operator bool() override {  // NOLINT
    return mock_operator_bool();
  }
  MOCK_METHOD(bool, mock_operator_bool, (), ());

  // EthernetClient metods:
  MOCK_METHOD(uint8_t, status, (), (override));
  MOCK_METHOD(uint8_t, getSocketNumber, (), (const, override));
};

}  // namespace test
}  // namespace mcunet

#endif  // MCUNET_EXTRAS_TEST_TOOLS_MOCK_ETHERNET_CLIENT_H_
