#include "extras/host/ethernet5500/ethernet_client.h"

#include "absl/log/check.h"
#include "absl/log/log.h"
#include "platform_network_interface.h"

// using ::mcunet_host::HostSockets;

using ::mcunet::PlatformNetworkInterface;

EthernetClient::EthernetClient(uint8_t sock) : sock_(sock) {}

int EthernetClient::connect(IPAddress ip, uint16_t port) {
  CHECK(false) << "EthernetClient::connect Unimplemented";
  return 0;
}

int EthernetClient::connect(const char *host, uint16_t port) {
  CHECK(false) << "EthernetClient::connect Unimplemented";
  return 0;
}

uint8_t EthernetClient::status() {
  return PlatformNetworkInterface::GetImplementationOrDie()->SocketStatus(
      sock_);
}

size_t EthernetClient::write(uint8_t value) {
  return PlatformNetworkInterface::GetImplementationOrDie()->Send(sock_, &value,
                                                                  1);
}

size_t EthernetClient::write(const uint8_t *buf, size_t size) {
  if (size > 2048) {
    size = 2048;
  }
  return PlatformNetworkInterface::GetImplementationOrDie()->Send(sock_, buf,
                                                                  size);
}

// Returns the number of bytes available for reading. It may not be possible
// to read that many bytes in one call to read.
int EthernetClient::available() {
  return PlatformNetworkInterface::GetImplementationOrDie()->AvailableBytes(
      sock_);
}

// Read one byte from the stream.
int EthernetClient::read() {
  uint8_t value;
  if (PlatformNetworkInterface::GetImplementationOrDie()->Recv(sock_, &value,
                                                               1) > 0) {
    return value;
  } else {
    return -1;
  }
}

// Read up to 'size' bytes from the stream, returns the number read.
int EthernetClient::read(uint8_t *buf, size_t size) {
  return PlatformNetworkInterface::GetImplementationOrDie()->Recv(sock_, buf,
                                                                  size);
}

// Returns the next available byte/
int EthernetClient::peek() {
  return PlatformNetworkInterface::GetImplementationOrDie()->Peek(sock_);
}

void EthernetClient::flush() {
  return PlatformNetworkInterface::GetImplementationOrDie()->Flush(sock_);
}

void EthernetClient::stop() {
  if (!PlatformNetworkInterface::GetImplementationOrDie()->CloseSocket(sock_)) {
    VLOG(1) << "EthernetClient::stop failed";
  }
}

uint8_t EthernetClient::connected() {
  return PlatformNetworkInterface::GetImplementationOrDie()
      ->SocketIsInTcpConnectionLifecycle(sock_);
}

EthernetClient::operator bool() { return connected() != 0; }
