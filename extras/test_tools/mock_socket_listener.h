#ifndef MCUNET_EXTRAS_TEST_TOOLS_MOCK_SOCKET_LISTENER_H_
#define MCUNET_EXTRAS_TEST_TOOLS_MOCK_SOCKET_LISTENER_H_

#include "connection.h"
#include "gmock/gmock.h"
#include "socket_listener.h"

namespace mcunet {
namespace test {

class MockSocketListener : public SocketListener {
 public:
  MOCK_METHOD(void, OnCanRead, (class mcunet::Connection &), (override));

  MOCK_METHOD(void, OnHalfClosed, (class mcunet::Connection &), (override));

  MOCK_METHOD(void, OnDisconnect, (), (override));
};

class MockServerSocketListener : public ServerSocketListener {
 public:
  MOCK_METHOD(void, OnCanRead, (class mcunet::Connection &), (override));

  MOCK_METHOD(void, OnHalfClosed, (class mcunet::Connection &), (override));

  MOCK_METHOD(void, OnDisconnect, (), (override));

  MOCK_METHOD(void, OnConnect, (class mcunet::Connection &), (override));
};

}  // namespace test
}  // namespace mcunet

#endif  // MCUNET_EXTRAS_TEST_TOOLS_MOCK_SOCKET_LISTENER_H_
