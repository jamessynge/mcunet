#ifndef MCUNET_EXTRAS_HOST_ETHERNET5500_HOST_SOCKET_INFO_H_
#define MCUNET_EXTRAS_HOST_ETHERNET5500_HOST_SOCKET_INFO_H_

// To simulate harware sockets, as provided by the WIZnet W5500 and similar
// chips, which can either be listening or connected, but not both,
// HostSocketInfo provides the ability to associate a small integer (the
// sock_num) to a host listener socket or host connection socket.
//
// QUESTION: Should we support AF_UNIX here, either instead of AF_INET, or in
// addition to? Since the purpose is for testing, AF_UNIX should be sufficient,
// but maybe it will be convenient to be a fully fledged network server.
//
// Author: james.synge@gmail.com

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#include <string>

namespace mcunet_host {

class HostSocketInfo {
 public:
  // These values match the values of the W5500 socket status register.
  static constexpr uint8_t kStatusClosed = 0;
  static constexpr uint8_t kStatusListening = 0x14;
  static constexpr uint8_t kStatusCloseWait = 0x1C;
  static constexpr uint8_t kStatusEstablished = 0x17;

  explicit HostSocketInfo(uint8_t sock_num);
  // Closes any open host socket (i.e. listener or connection).
  ~HostSocketInfo();

  // Returns a string describing the instance.
  std::string ToString() const;

  //////////////////////////////////////////////////////////////////////////////
  // Methods getting the status of a socket.

  // Returns true if there is neither a listener nor a connection socket nor a
  // tcp port (i.e. it wasn't previously used for listening, then a connection
  // which has been 'recently' closed).
  bool IsUnused();

  // Returns the non-zero port number if the socket is listening for TCP
  // connections to a port.
  uint16_t IsTcpListener();

  // Returns true if there is an open connection socket.
  // TODO(jamessynge): This should probably be changed to "there is a connection
  // socket, and the peer hasn't closed it from their end."
  bool IsConnected();

  // Returns true if there is an open connection socket and the peer has
  // half-closed the connection.
  bool IsConnectionHalfClosed();

  // Returns true if there is an open connection socket and either there is data
  // available to be read immediately, or there isn't data but it appears that
  // the peer hasn't half-closed the connection so there may be more data in the
  // future.
  bool CanReadFromConnection();

  // Returns true if there is neither a listener nor a connection socket.
  // TODO(jamessynge): Need to consider whether this matches the semantic we
  // need for PlatformNetworkInterface.
  bool IsClosed();

  // Returns the implementation defined status value for the specified socket.
  // TODO(jamessynge): Try to eliminate the same named method from
  // PlatformNetworkInterface, at which point it can be removed here.
  uint8_t SocketStatus();

  //////////////////////////////////////////////////////////////////////////////
  // Methods modifying sockets. So far these are all related to being a TCP
  // server, but eventually there should be methods for being a client, for UDP,
  // and of course for reading and writing data.

  // Start (or continue) listening for new TCP connections on 'tcp_port';
  // if currently connected to a peer, disconnect. Returns true if successful.
  bool InitializeTcpListener(uint16_t new_tcp_port);

  // If there is a new connection from a peer available to be accepted, do so
  // and return true; else returns false.
  bool AcceptConnection();

  // Half close (shutdown in Posix terms) the connection socket; i.e. close it
  // for writing, but not for reading. Signals EOF to a Posix Sockets API peer
  // when it attempts to read after reading all bytes already written. Returns
  // true if there is a connection open and it has been successfully
  // disconnected, else false.
  bool DisconnectConnectionSocket();

  // Completely close the connection socket, if there is an open connection
  // socket.
  void CloseConnectionSocket();

  // Close the listener socket (i.e. stop listening for new connections), if
  // there is an open listener socket.
  void CloseListenerSocket();

  // Set the host socket whose fd is provided to be non-blocking. Returns true
  // if successful.
  static bool SetNonBlocking(int fd);

  //////////////////////////////////////////////////////////////////////////////
  // Methods using open sockets.

  // Sends on an open connection. Returns the number of bytes sent, or -1 if an
  // error is encountered.
  ssize_t Send(const uint8_t *buf, size_t len);

  // Flush any bytes queued in the socket for sending.
  void Flush();

  // Returns the number of bytes available for reading from the socket; if an
  // error occurred, -1 is returned; if all bytes written by the peer have been
  // read, and the peer has performed an orderly shutdown of writing, then 0 is
  // returned, indicating EOF; however, 0 will also be returned if that is the
  // number of bytes available to read from a fully open connection.
  ssize_t AvailableBytes();

  // Returns the first available byte from the socket, or -1 if there is no byte
  // available, including if the connection is not open.
  int Peek();

  // Receives from an open connection. Returns the number of bytes received and
  // copied into the buffer; if an error occurred, -1 is returned; if the peer
  // has performed an orderly shutdown of writing, then 0 is returned,
  // indicating EOF.
  ssize_t Recv(uint8_t *buf, size_t len);

 private:
  ssize_t RecvInternal(uint8_t *buf, size_t len, bool peek);

  const int sock_num_;

  // IFF listening, these three are at non-default values.
  int listener_socket_fd_{-1};
  uint16_t tcp_port_{0};
  // Port number to which the requested port has been mapped.
  uint16_t mapped_tcp_port_{0};

  // IFF a connection has been accepted, set to a non-default value (>= 0).
  int connection_socket_fd_{-1};
  bool can_write_to_connection_{false};
  bool can_read_from_connection_{false};
};

}  // namespace mcunet_host

#endif  // MCUNET_EXTRAS_HOST_ETHERNET5500_HOST_SOCKET_INFO_H_
