#ifndef MCUNET_EXTRAS_TEST_TOOLS_FAKE_WRITE_BUFFERED_CONNECTION_H_
#define MCUNET_EXTRAS_TEST_TOOLS_FAKE_WRITE_BUFFERED_CONNECTION_H_

// Provides an implementation of WriteBufferedConnection which can be used for
// testing code that uses WriteBufferedConnection or Connection.
//
// Author: james.synge@gmail.com

#include <stddef.h>
#include <stdint.h>

#include <array>
#include <limits>

#include "extras/host/arduino/client.h"
#include "gtest/gtest.h"
#include "write_buffered_connection.h"

namespace mcunet {
namespace test {

class FakeWriteBufferedConnection : public WriteBufferedConnection {
 public:
  FakeWriteBufferedConnection(Client& client, uint8_t sock_num,
                              uint8_t* write_buffer, uint8_t write_buffer_limit)
      : WriteBufferedConnection(write_buffer, write_buffer_limit, client),
        sock_num_(sock_num) {}

  template <size_t N>
  FakeWriteBufferedConnection(Client& client, uint8_t sock_num,
                              std::array<uint8_t, N>& write_buffer)
      : FakeWriteBufferedConnection(client, sock_num, write_buffer.data(),
                                    static_cast<uint8_t>(N)) {
    static_assert(N <= std::numeric_limits<uint8_t>::max());
  }

  void close() override {
    EXPECT_LE(close_count_, 1);
    if (close_count_ == 0) {
      client().stop();
    }
    close_count_++;
  }

  uint8_t connected() override {
    if (close_count_ == 0) {
      return WriteBufferedConnection::connected();
    }
    return 0;
  }

  uint8_t sock_num() const override { return sock_num_; }

  // We make setWriteError public for testing.
  using WriteBufferedConnection::setWriteError;

 private:
  const uint8_t sock_num_;
  size_t close_count_{0};
};

}  // namespace test
}  // namespace mcunet

#endif  // MCUNET_EXTRAS_TEST_TOOLS_FAKE_WRITE_BUFFERED_CONNECTION_H_
