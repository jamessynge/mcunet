#ifndef MCUNET_SRC_CONNECTION_H_
#define MCUNET_SRC_CONNECTION_H_

// Connection is like Arduino's Client class, but doesn't include the connect
// method; i.e. a Connection is used to represent an existing (TCP) connection,
// not to create one. This is useful for implementing a server where we want to
// work with connections initiated by a remote client.
//
// Author: james.synge@gmail.com

#include <Stream.h>
#include <stddef.h>

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

  // Declare the base class read method in this class, so that read is an
  // overloaded method name in this class, rather than the following method
  // hiding the base class method.
  using Stream::read;

  // Reads up to 'size' bytes into buf, stopping early if there are no more
  // bytes available to read from the connection. Returns the number of bytes
  // read, if any; if the peer has closed their side for writing (half-closed),
  // then 0 is returned to indicate EOF; if no bytes are available currently on
  // a fully open connection, then -1 is returned.
  //
  // The default implementation uses `int Stream::read()` to read one byte at a
  // time, but ideally a subclass overrides it with a more efficient
  // implementation.
  virtual int read(uint8_t *buf, size_t size) = 0;

  // Returns the hardware socket number of this connection. This is exposed
  // primarily to support debugging.
  virtual uint8_t sock_num() const = 0;
};

}  // namespace mcunet

#endif  // MCUNET_SRC_CONNECTION_H_
