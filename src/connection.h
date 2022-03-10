#ifndef MCUNET_SRC_CONNECTION_H_
#define MCUNET_SRC_CONNECTION_H_

// Connection is like Arduino's Client class, but doesn't include the connect
// method; i.e. a Connection is used to represent an existing (TCP) connection,
// not to create one. This is useful for implementing a server where we want to
// work with connections initiated by a remote client.
//
// Author: james.synge@gmail.com

#include "platform_ethernet.h"

namespace mcunet {

// Extends Arduino's Stream with methods for closing a connection and checking
// whether it has been closed.
class Connection : public Stream {
 public:
  // Returns the number of readable bytes. If not connected, returns -1.
  int available() override = 0;

  // Close the connection (fully, not half-closed).
  virtual void close() = 0;

  // Returns true if the connection is either readable or writeable.
  virtual bool connected() const = 0;

  // Returns true if the peer (e.g. client of the server) half-closed its
  // connection for writing (e.g. by calling shutdown(fd, SHUT_WR), but is
  // waiting for us to write. This is apparently now an unlikely state because
  // clients won't typically close until they've read the full response. See
  // https://www.excentis.com/blog/tcp-half-close-cool-feature-now-broken.
  virtual bool peer_half_closed() const = 0;

  // Reads up to 'size' bytes into buf, stopping early if there are no more
  // bytes available to read from the connection. Returns the number of bytes
  // read. The default implementation uses `int Stream::read()` to read one byte
  // at a time.
  virtual size_t read(uint8_t *buf, size_t size);

  using Stream::read;

  // Returns the hardware socket number of this connection. This is exposed
  // primarily to support debugging.
  virtual uint8_t sock_num() const = 0;

  // Returns true if there is a write error or if the connection is broken.
  virtual bool hasWriteError();
};

// An abstract implementation of Connection that delegates to a Client instance
// provided by a subclass. To produce a concrete instance, some subclass will
// also need to implement the public methods close() and connected(), and the
// protected method client().
// TODO(jamessynge): Determine whether this should be retained. It has been
// superceded by WriteBufferedWrappedClientConnection.
class WrappedClientConnection : public Connection {
 public:
  size_t write(uint8_t b) override;
  size_t write(const uint8_t *buf, size_t size) override;
  int availableForWrite() override;
  int available() override;
  int read() override;
  size_t read(uint8_t *buf, size_t size) override;
  int peek() override;
  void flush() override;

 protected:
  virtual Client &client() const = 0;
};

// An abstract implementation of Connection that delegates to a Client instance
// provided by a subclass, adding buffering of writes. I added this because I
// found the performance to be very slow without it, and realized this was
// because each character or string being printed to the EthernetClient was
// resulting in an SPI transaction. By buffering up a bunch of smaller strings
// we amortize the cost setting up the transaction. Note that I've not measured
// the optimal size of the buffer, nor have I investigated performing any kind
// of async SPI... it doesn't seem necessary for Tiny Alpaca Server and would
// require more buffer management.
// TODO(jamessynge): Consider shortening the name of this class. It's unwieldy.
class WriteBufferedWrappedClientConnection : public Connection {
 public:
  WriteBufferedWrappedClientConnection(uint8_t *write_buffer,
                                       uint8_t write_buffer_limit);

  size_t write(uint8_t b) override;
  size_t write(const uint8_t *buf, size_t size) override;
  int availableForWrite() override;
  int available() override;
  int read() override;
  size_t read(uint8_t *buf, size_t size) override;
  int peek() override;
  void flush() override;

 protected:
  virtual Client &client() const = 0;
  uint8_t write_buffer_size() const { return write_buffer_size_; }

 private:
  void FlushIfFull();

  uint8_t *const write_buffer_;
  const uint8_t write_buffer_limit_;
  uint8_t write_buffer_size_;
};

}  // namespace mcunet

#endif  // MCUNET_SRC_CONNECTION_H_
