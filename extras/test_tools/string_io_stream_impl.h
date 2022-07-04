#ifndef MCUNET_EXTRAS_TEST_TOOLS_STRING_IO_STREAM_IMPL_H_
#define MCUNET_EXTRAS_TEST_TOOLS_STRING_IO_STREAM_IMPL_H_

// Implements Connection for testing usages of Connection; reads from a
// std::string_view, and writes to a std::string.
//
// Author: james.synge@gmail.com

#include <stddef.h>
#include <stdint.h>

#include <algorithm>
#include <cstring>
#include <string>
#include <string_view>

#include "connection.h"
#include "mcucore/extras/host/arduino/stream.h"

namespace mcunet {
namespace test {

template <class BASE = Stream>
class StringIoStreamImpl : public BASE {
 public:
  StringIoStreamImpl(uint8_t sock_num, std::string_view input)
      : input_buffer_(input),
        input_view_(input_buffer_),
        is_open_(true),
        sock_num_(sock_num) {}

  // Test support methods.

  std::string_view remaining_input() const { return input_view_; }

  const std::string& output() const { return output_; }

  // Print methods:

  size_t write(uint8_t b) override {
    if (!is_open_) {
      return -1;
    }
    output_.push_back(static_cast<char>(b));
    return 1;
  }

  size_t write(const uint8_t* buffer, size_t size) override {
    if (!is_open_) {
      return -1;
    }
    output_.append(reinterpret_cast<const char*>(buffer), size);
    return size;
  }

  // Stream methods:

  int available() override { return is_open_ ? input_view_.size() : -1; }

  int read() override {
    if (!is_open_) {
      return -1;
    } else if (input_view_.empty()) {
      return -1;
    }
    char c = input_view_[0];
    input_view_.remove_prefix(1);
    return c;
  }

  int peek() override {
    if (!is_open_) {
      return -1;
    } else if (input_view_.empty()) {
      return -1;
    } else {
      return input_view_[0];
    }
  }

  // Some Client methods; these are declared as virtual rather than as
  // overriding so that BASE does not have to be Client.

  virtual int read(uint8_t* buf, size_t size) {
    if (!is_open_) {
      return -1;
    }
    size = std::min(size, input_view_.size());
    if (size > 0) {
      std::memcpy(buf, input_view_.data(), size);
      input_view_.remove_prefix(size);
    }
    return size;
  }

  virtual void stop() { is_open_ = false; }

  virtual uint8_t connected() { return is_open_; }

  // Some mcunet::Connection methods; these are declared as virtual rather than
  // as overriding so that BASE does not have to be Connection.

  virtual void close() { is_open_ = false; }

  virtual uint8_t sock_num() const { return sock_num_; }

 private:
  const std::string input_buffer_;
  std::string_view input_view_;
  std::string output_;
  bool is_open_;
  const uint8_t sock_num_;
};

using StringIoStream = StringIoStreamImpl<::Stream>;
using StringIoClient = StringIoStreamImpl<::Client>;
using StringIoConnection = StringIoStreamImpl<::mcunet::Connection>;

}  // namespace test
}  // namespace mcunet

#endif  // MCUNET_EXTRAS_TEST_TOOLS_STRING_IO_STREAM_IMPL_H_
