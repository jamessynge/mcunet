#ifndef MCUNET_EXTRAS_HOST_ARDUINO_CLIENT_H_
#define MCUNET_EXTRAS_HOST_ARDUINO_CLIENT_H_

// Represents a stream connected to a server.

#include <stddef.h>
#include <stdint.h>

#include "extras/host/arduino/ip_address.h"
#include "mcucore/extras/host/arduino/stream.h"

class Client : public Stream {
 public:
  virtual int connect(IPAddress ip, uint16_t port) = 0;
  virtual int connect(const char *host, uint16_t port) = 0;
  using Stream::read;
  virtual int read(uint8_t *buf, size_t size) = 0;
  virtual void stop() = 0;
  virtual uint8_t connected() = 0;
  virtual operator bool() = 0;  // NOLINT
};

#endif  // MCUNET_EXTRAS_HOST_ARDUINO_CLIENT_H_
