#ifndef MCUNET_EXTRAS_TEST_TOOLS_MOCK_SOCKET_LISTENER_H_
#define MCUNET_EXTRAS_TEST_TOOLS_MOCK_SOCKET_LISTENER_H_

// Mock classes for SocketListener and ServerSocketListener.
//
// The only types used here are those of the class(es) being mocked and those
// used in the methods being mocked. Even though IWYU would call for more
// headers to be included, I've chosen here to limit the includes in headers
// like this to gmock.h and the header(s) of the class(es) being mocked.
//
// Author: james.synge@gmail.com

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
  // SocketListener methods:
  MOCK_METHOD(void, OnCanRead, (class mcunet::Connection &), (override));
  MOCK_METHOD(void, OnHalfClosed, (class mcunet::Connection &), (override));
  MOCK_METHOD(void, OnDisconnect, (), (override));

  // ServerSocketListener methods:
  MOCK_METHOD(void, OnConnect, (class mcunet::Connection &), (override));
};

}  // namespace test
}  // namespace mcunet

#endif  // MCUNET_EXTRAS_TEST_TOOLS_MOCK_SOCKET_LISTENER_H_
