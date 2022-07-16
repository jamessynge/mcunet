#ifndef MCUNET_EXTRAS_TEST_TOOLS_MOCK_ETHERNET_UDP_H_
#define MCUNET_EXTRAS_TEST_TOOLS_MOCK_ETHERNET_UDP_H_

// Mock class for EthernetUDP. While EthernetUDP is in the root namespace, I've
// chosen to place the mock class into a test namespace so that it is really
// clear that it is for use in tests; in some cases this avoids the need for a
// using declaration.
//
// The only types used here are those of the class(es) being mocked and those
// used in the methods being mocked. Even though IWYU would call for more
// headers to be included, I've chosen here to limit the includes in headers
// like this to gmock.h and the header(s) of the class(es) being mocked.

#include <stddef.h>
#include <stdint.h>

#include "extras/host/arduino/ip_address.h"
#include "extras/host/ethernet5500/ethernet_udp.h"
#include "gmock/gmock.h"

namespace mcunet {
namespace test {

class MockEthernetUDP : public EthernetUDP {
 public:
  // Print methods:
  MOCK_METHOD(size_t, write, (uint8_t), (override));
  MOCK_METHOD(size_t, write, (const uint8_t *, size_t), (override));
  MOCK_METHOD(void, flush, (), (override));

  // Stream methods:
  MOCK_METHOD(int, available, (), (override));
  MOCK_METHOD(int, read, (), (override));
  MOCK_METHOD(int, peek, (), (override));

  // EthernetUDP methods:
  MOCK_METHOD(uint8_t, begin, (uint16_t), (override));
  MOCK_METHOD(int, beginPacket, (class IPAddress, uint16_t), (override));
  MOCK_METHOD(int, beginPacket, (const char *, uint16_t), (override));
  MOCK_METHOD(int, endPacket, (), (override));
  MOCK_METHOD(int, parsePacket, (), (override));
  MOCK_METHOD(int, read, (uint8_t *, size_t), (override));
  MOCK_METHOD(int, read, (char *, size_t), (override));
  MOCK_METHOD(IPAddress, remoteIP, (), (override));
  MOCK_METHOD(void, remoteIP, (uint8_t *), (override));
  MOCK_METHOD(uint16_t, remotePort, (), (override));
  MOCK_METHOD(void, remoteMAC, (uint8_t *), (override));
};

}  // namespace test
}  // namespace mcunet

#endif  // MCUNET_EXTRAS_TEST_TOOLS_MOCK_ETHERNET_UDP_H_
