#ifndef MCUNET_SRC_WRITE_BUFFERED_CONNECTION_H_
#define MCUNET_SRC_WRITE_BUFFERED_CONNECTION_H_

// WriteBufferedConnection is an abstract implementation of Connection that
// buffers writes, and delegates reading and writing (after buffering) to a
// Client instance provided by a subclass, adding buffering of writes. I added
// this because I found the performance to be very slow without it, and realized
// this was because each character or string being printed to the EthernetClient
// was resulting in an SPI transaction. By buffering up a bunch of smaller
// strings we amortize the cost setting up the transaction. Note that I've not
// measured the optimal size of the buffer, nor have I investigated performing
// any kind of async SPI... it doesn't seem necessary for Tiny Alpaca Server and
// would require more buffer management.

#include <stddef.h>
#include <stdint.h>

#include "connection.h"
#include "extras/host/arduino/client.h"  // pragma: keep extras include

namespace mcunet {

class WriteBufferedConnection : public Connection {
 public:
  // The write error value will be set to kBlockedFlush if unable to write any
  // bytes (in FlushInternal).
  static constexpr int kBlockedFlush = 1234;

  WriteBufferedConnection(uint8_t* write_buffer, uint8_t write_buffer_limit,
                          Client& client);
  // Writes any data accumulated in the write buffer to the underlying client.
  // Does NOT call client_.flush().
  ~WriteBufferedConnection() override;

  using Connection::write;
  size_t write(uint8_t b) override;
  size_t write(const uint8_t* buf, size_t size) override;
  int availableForWrite() override;
  int available() override;
  int read() override;
  int read(uint8_t* buf, size_t size) override;
  using Connection::read;
  int peek() override;
  void flush() override;
  uint8_t connected() override;

 protected:
  uint8_t write_buffer_size() const { return write_buffer_size_; }
  Client& client() { return client_; }

 private:
  // Flush the (non-empty) buffer to the client. There may or may not be a write
  // error already recorded. Returns true if successful, false if an error is
  // detected while or before writing to client_.
  bool FlushInternal();

  uint8_t* const write_buffer_;
  const uint8_t write_buffer_limit_;
  uint8_t write_buffer_size_;
  Client& client_;
};

}  // namespace mcunet

#endif  // MCUNET_SRC_WRITE_BUFFERED_CONNECTION_H_
