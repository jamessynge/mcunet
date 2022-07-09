#ifndef MCUNET_EXTRAS_TEST_TOOLS_MOCK_CONNECTION_H_
#define MCUNET_EXTRAS_TEST_TOOLS_MOCK_CONNECTION_H_

#include "connection.h"

namespace mcunet {
namespace test {

class MockConnection : public ::mcunet::Connection {
 public:
  virtual ~MockConnection();

  // Print methods:
  MOCK_METHOD(size_t, write, (uint8_t), (override));
  MOCK_METHOD(size_t, write, (const uint8_t *, size_t), (override));
  MOCK_METHOD(void, flush, (), (override));

  // Stream methods:
  MOCK_METHOD(int, available, (), (override));
  MOCK_METHOD(int, read, (), (override));
  MOCK_METHOD(int, peek, (), (override));

  // Connection methods:
  MOCK_METHOD(int, read, (uint8_t *, size_t), (override));
  MOCK_METHOD(void, stop, (), (override));
  MOCK_METHOD(uint8_t, connected, (), (override));

  operator bool() override {  // NOLINT
    return mock_operator_bool();
  }
  MOCK_METHOD(bool, mock_operator_bool, (), ());
};

}  // namespace test
}  // namespace mcunet

#endif  // MCUNET_EXTRAS_TEST_TOOLS_MOCK_CONNECTION_H_
