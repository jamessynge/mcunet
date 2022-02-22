#ifndef MCUNET_EXTRAS_TEST_TOOLS_MOCK_CLIENT_H_
#define MCUNET_EXTRAS_TEST_TOOLS_MOCK_CLIENT_H_

#include <stdint.h>

#include <cstddef>

#include "extras/host/arduino/client.h"
#include "extras/host/arduino/ip_address.h"
#include "gmock/gmock.h"
#include "mcucore/extras/test_tools/mock_stream.h"

namespace test {
class MockClient : public Client, public MockStream {
 public:
  using MockStream::read;

  MOCK_METHOD(int, connect, (class IPAddress, uint16_t), (override));
  MOCK_METHOD(int, connect, (const char *, uint16_t), (override));
  MOCK_METHOD(int, read, (uint8_t *, size_t), (override));
  MOCK_METHOD(void, stop, (), (override));
  MOCK_METHOD(uint8_t, connected, (), (override));

  operator bool() override {  // NOLINT
    return mock_operator_bool();
  }
  MOCK_METHOD(bool, mock_operator_bool, (), ());
};
}  // namespace test

#endif  // MCUNET_EXTRAS_TEST_TOOLS_MOCK_CLIENT_H_
