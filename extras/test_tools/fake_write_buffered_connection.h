#ifndef MCUNET_EXTRAS_TEST_TOOLS_FAKE_WRITE_BUFFERED_CONNECTION_H_
#define MCUNET_EXTRAS_TEST_TOOLS_FAKE_WRITE_BUFFERED_CONNECTION_H_

// TODO(jamessynge): Describe why this file exists/what it provides.

#include <array>
#include <cstdint>

#include "connection.h"
#include "extras/host/arduino/client.h"

namespace mcunet {
namespace test {

class FakeWriteBufferedWrappedClientConnection
    : public WriteBufferedWrappedClientConnection {
 public:
  FakeWriteBufferedWrappedClientConnection(Client& client, uint8_t sock_num,
                                           uint8_t* write_buffer,
                                           uint8_t write_buffer_limit)
      : WriteBufferedWrappedClientConnection(write_buffer, write_buffer_limit),
        client_(client),
        sock_num_(sock_num) {}

  template <size_t N>
  FakeWriteBufferedWrappedClientConnection(Client& client,
                                           std::array<uint8_t, N> write_buffer)
      : FakeWriteBufferedWrappedClientConnection(client, write_buffer.data(),
                                                 N) {}

  void close() override {
    if (close_count_ == 0) {
      client_.stop();
    }
    close_count_++;
  }

  uint8_t connected() override {
    return close_count_ == 0 && client_.connected();
  }

  uint8_t sock_num() const override { return sock_num_; }

 private:
  Client& client() const override { return client_; }

  Client& client_;
  const uint8_t sock_num_;
  size_t close_count_{0};
};

}  // namespace test
}  // namespace mcunet

#endif  // MCUNET_EXTRAS_TEST_TOOLS_FAKE_WRITE_BUFFERED_CONNECTION_H_
