#include "extras/test_tools/string_io_stream_impl.h"

#include <stdint.h>

#include "gtest/gtest.h"

namespace mcunet {
namespace test {
namespace {

template <typename T>
class StringIoStreamTest : public testing::Test {};

TYPED_TEST_SUITE_P(StringIoStreamTest);

TYPED_TEST_P(StringIoStreamTest, DoNothing) {
  StringIoStream conn(3, "");
  EXPECT_TRUE(conn.connected());
  EXPECT_EQ(conn.sock_num(), 3);
  EXPECT_EQ(conn.available(), 0);
  EXPECT_EQ(conn.remaining_input(), "");
  EXPECT_EQ(conn.output(), "");
}

TYPED_TEST_P(StringIoStreamTest, WriteAndReadAndClose) {
  StringIoStream conn(1, "DEFG");

  EXPECT_EQ(conn.print("ab"), 2);
  EXPECT_EQ(conn.print("cd"), 2);
  EXPECT_EQ(conn.output(), "abcd");

  EXPECT_EQ(conn.available(), 4);
  EXPECT_EQ(conn.read(), 'D');
  EXPECT_EQ(conn.available(), 3);
  uint8_t buf[10] = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};
  EXPECT_EQ(conn.read(buf, 2), 2);
  EXPECT_EQ(buf[0], 'E');
  EXPECT_EQ(buf[1], 'F');
  EXPECT_EQ(buf[2], ' ');
  EXPECT_EQ(conn.available(), 1);

  // Make sure the connection doesn't do anything once closed.
  conn.close();
  EXPECT_FALSE(conn.connected());
  EXPECT_EQ(conn.available(), -1);
  EXPECT_EQ(conn.read(), -1);
  EXPECT_EQ(conn.read(buf, 10), -1);
  EXPECT_EQ(conn.print("xyz"), -1);

  EXPECT_EQ(conn.sock_num(), 1);
  EXPECT_EQ(conn.remaining_input(), "G");
  EXPECT_EQ(conn.output(), "abcd");
}

TYPED_TEST_P(StringIoStreamTest, ReadAll) {
  StringIoStream conn(1, "DEFG");

  EXPECT_EQ(conn.available(), 4);
  uint8_t buf[10] = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};
  EXPECT_EQ(conn.read(buf, 10), 4);
  EXPECT_EQ(buf[0], 'D');
  EXPECT_EQ(buf[1], 'E');
  EXPECT_EQ(buf[2], 'F');
  EXPECT_EQ(buf[3], 'G');
  EXPECT_EQ(buf[4], ' ');
  EXPECT_EQ(conn.available(), 0);
}

REGISTER_TYPED_TEST_SUITE_P(StringIoStreamTest, DoNothing, WriteAndReadAndClose,
                            ReadAll);

INSTANTIATE_TYPED_TEST_SUITE_P(ForStream, StringIoStreamTest, StringIoStream);
INSTANTIATE_TYPED_TEST_SUITE_P(ForClient, StringIoStreamTest, StringIoClient);
INSTANTIATE_TYPED_TEST_SUITE_P(ForConnection, StringIoStreamTest,
                               StringIoConnection);

}  // namespace
}  // namespace test
}  // namespace mcunet
