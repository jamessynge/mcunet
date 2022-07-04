#ifndef MCUNET_SRC_CONNECTION_H_
#define MCUNET_SRC_CONNECTION_H_

// Connection is like Arduino's Client class, but doesn't include the connect
// method; i.e. a Connection is used to represent an existing (TCP) connection,
// not to create one. This is useful for implementing a server where we want to
// work with connections initiated by a remote client.
//
// Author: james.synge@gmail.com

#include "platform_network.h"

namespace mcunet {

// Extends Arduino's Stream with methods for closing a connection and checking
// whether it has been closed.
class Connection : public Stream {
 public:
#if MCU_HOST_TARGET
  ~Connection() override;
#endif
#if MCU_EMBEDDED_TARGET
  // Arduino doesn't define dtors, so we need to here in order to allow
  // subclasses to specify that they override some base class dtor.
  virtual ~Connection();
#endif

  // Returns the number of readable bytes. If not connected, returns -1.
  int available() override = 0;

  // Close the connection (fully, not half-closed).
  virtual void close() = 0;

  // Returns true (non-zero) if the connection is either readable or writeable,
  // else returns false (zero). The return type is uint8_t, instead of bool,
  // simply because that is the return type of Arduino's Client::connected()
  // method. The same applies to the lack of a const qualifier on the method.
  virtual uint8_t connected() = 0;

  // Reads up to 'size' bytes into buf, stopping early if there are no more
  // bytes available to read from the connection. Returns the number of bytes
  // read, if any; if the peer has closed their side for writing (half-closed),
  // then 0 is returned to indicate EOF; if no bytes are available currently on
  // a fully open connection, then -1 is returned.
  //
  // The default implementation uses `int Stream::read()` to read one byte at a
  // time, but ideally a subclass overrides it with a more efficient
  // implementation.
  virtual int read(uint8_t *buf, size_t size);

  // Declare the base class `int read()` method in this class, so that read is
  // an overloaded method name in this class, rather than the prior method being
  // an override.
  using Stream::read;

  // Returns the hardware socket number of this connection. This is exposed
  // primarily to support debugging.
  virtual uint8_t sock_num() const = 0;

  // Returns true if there is a write error has been recorded.
  virtual bool HasWriteError();

  // Returns true if there is no write error recorded and connected() is true.
  virtual bool IsOkToWrite();
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
class WriteBufferedWrappedClientConnection : public Connection {
 public:
  WriteBufferedWrappedClientConnection(uint8_t *write_buffer,
                                       uint8_t write_buffer_limit);

  size_t write(uint8_t b) override;
  size_t write(const uint8_t *buf, size_t size) override;
  int availableForWrite() override;
  int available() override;
  int read() override;
  int read(uint8_t *buf, size_t size) override;
  int peek() override;
  void flush() override;
  uint8_t connected() override;
  bool HasWriteError() override;
  bool IsOkToWrite() override;

 protected:
  virtual Client &client() const = 0;
  uint8_t write_buffer_size() const { return write_buffer_size_; }

 private:
  void FlushIfFull();
  size_t WriteToClient(const uint8_t *buf, size_t size);

  uint8_t *const write_buffer_;
  const uint8_t write_buffer_limit_;
  uint8_t write_buffer_size_;
};

}  // namespace mcunet

#endif  // MCUNET_SRC_CONNECTION_H_
