#include "server_socket.h"

#include <stdint.h>

#include "extras/test_tools/mock_socket_listener.h"
#include "gtest/gtest.h"

namespace mcunet {
namespace test {
namespace {

const uint16_t kTcpPort = 9999;

class ServerSocketTest : public testing::Test {
 protected:
  ServerSocketTest() : server_socket_(kTcpPort, mock_listener_) {}
  void SetUp() override {}

  MockServerSocketListener mock_listener_;
  ServerSocket server_socket_;
};

TEST_F(ServerSocketTest, NewInstanceTest) {
  EXPECT_FALSE(server_socket_.HasSocket());
  EXPECT_FALSE(server_socket_.IsConnected());
  EXPECT_TRUE(server_socket_.ReleaseSocket());
  server_socket_.SocketLost();
}

}  // namespace
}  // namespace test
}  // namespace mcunet
