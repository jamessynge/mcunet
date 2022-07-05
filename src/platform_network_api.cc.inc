// ISSUE: Is it reasonable to add some hardware setup methods, such as:
// * Set the maximum number of sockets, at least query that value.
// * Reset all sockets (e.g. close them all/stop listening).

////////////////////////////////////////////////////////////////////////////////
// Methods getting the status of a socket.

// Finds a hardware socket that is closed, and returns its socket number.
// Returns -1 if there is no such socket.
MCUNET_PNAPI_METHOD(int, FindUnusedSocket, ());

// Returns the non-zero port number if the socket is listening for TCP
// connections to a port.
MCUNET_PNAPI_METHOD(uint16_t, SocketIsTcpListener, (uint8_t sock_num));

// Returns true if the hardware socket is being used for TCP and is not
// LISTENING; if so, then it is best not to repurpose the hardware socket.
MCUNET_PNAPI_METHOD(bool, SocketIsInTcpConnectionLifecycle, (uint8_t sock_num));

// Returns true if the peer has closed their end for writing, but we've still
// got our end open for writing. This should not be true if there is still data
// available for us to read (e.g. buffered at our end).
//
// With the Posix Socket API, a connection can be half-closed by calling
//
//    shutdown(fd, SHUT_WR)
//
// where fd is the socket's file descriptor.
//
// This may not occur much because:
// a) HTTP client libraries aren't likely to half-close a connection while they
//    still have a request in flight; instead they'll wait until they'd consumed
//    the response before deciding the close the connection.
// b) For WAN connections, various network appliances (e.g. NAT or firewall
//    devices) may break half-closed connections in order to eliminate the
//    table entries associated with connections they (erroneously?) treat as at
//    the end of their lives. For more info, see:
//      https://www.excentis.com/blog/tcp-half-close-cool-feature-now-broken
MCUNET_PNAPI_METHOD(bool, SocketIsHalfClosed, (uint8_t sock_num));

// Returns true if the socket is completely closed (not in use for any purpose).
MCUNET_PNAPI_METHOD(bool, SocketIsClosed, (uint8_t sock_num));

// Returns the implementation defined status value for the specified socket.
// TODO(jamessynge): Try to eliminate this method, and the methods such as
// StatusIsOpen below, with the aim of not exposing application code to
// hardware/implementation specific status types and values.
MCUNET_PNAPI_METHOD(uint8_t, SocketStatus, (uint8_t sock_num));

////////////////////////////////////////////////////////////////////////////////
// Methods modifying sockets. So far these are all related to being a TCP
// server, but eventually there should be methods for being a client, for UDP,
// and of course for reading and writing data.

// Set socket 'sock_num' to listen for new TCP connections on port 'tcp_port',
// regardless of what that socket is doing now. Returns true if able to do so;
// false if not (e.g. if sock_num or tcp_port is invalid).
MCUNET_PNAPI_METHOD(bool, InitializeTcpListenerSocket,
                    (uint8_t sock_num, uint16_t tcp_port));

// Accept the pending new connection on socket 'sock_num', if the socket is
// currently a TCP listener socket with a pending connection. Returns true if
// there is such a new connection, otherwise false.
MCUNET_PNAPI_METHOD(bool, AcceptConnection, (uint8_t sock_num));

// Initiates a DISCONNECT of a TCP socket.
MCUNET_PNAPI_METHOD(bool, DisconnectSocket, (uint8_t sock_num));

// Forces a socket to be closed, with no packets sent out.
MCUNET_PNAPI_METHOD(bool, CloseSocket, (uint8_t sock_num));

////////////////////////////////////////////////////////////////////////////////
// Methods using open sockets.

// Sends on an open connection. Returns the number of bytes sent, or -1 if an
// error is encountered. Fewer than len bytes may be written under various
// circumstances; for example, the W5500 library (copied in the Ethernet5500
// library) imposes a 2048 byte limit.
MCUNET_PNAPI_METHOD(ssize_t, Send,
                    (uint8_t sock_num, const uint8_t* buf, size_t len));

// Flush any bytes queued in the socket for sending.
MCUNET_PNAPI_METHOD(void, Flush, (uint8_t sock_num));

// Returns the number of bytes available for reading from the socket; if an
// error occurred, -1 is returned; if all bytes written by the peer have been
// read, and the peer has performed an orderly shutdown of writing, then 0 is
// returned, indicating EOF; however, 0 will also be returned if that is the
// number of bytes available to read from a fully open connection.
MCUNET_PNAPI_METHOD(ssize_t, AvailableBytes, (uint8_t sock_num));

// Returns the first available byte on the specified socket, or -1 if there is
// no byte available, including if the connection is not open.
MCUNET_PNAPI_METHOD(int, Peek, (uint8_t sock_num));

// Receives from an open connection. Returns the number of bytes received and
// copied into the buffer; if an error occurred, -1 is returned; if the peer has
// performed an orderly shutdown of writing, then 0 is returned, indicating EOF.
MCUNET_PNAPI_METHOD(ssize_t, Recv,
                    (uint8_t sock_num, uint8_t* buf, size_t len));

////////////////////////////////////////////////////////////////////////////////
// Methods for checking the interpretation of the status value.
// TODO(jamessynge): Try these methods, and the SocketStatus method, with the
// aim of not exposing application code to hardware/implementation specific
// status types and values. One option is to return an implementation
// independent enum value, e.g. kTcpOpen, kTcpHalfClosed, kTcpListening.

// Returns true if the status indicates that the TCP connection is at least
// half-open.
MCUNET_PNAPI_METHOD(bool, StatusIsOpen, (uint8_t status));

// Returns true if the status indicates that the TCP connection is half-open.
MCUNET_PNAPI_METHOD(bool, StatusIsHalfClosed, (uint8_t status));

// Returns true if the status indicates that the TCP connection is in the
// process of closing (e.g. FIN_WAIT).
MCUNET_PNAPI_METHOD(bool, StatusIsClosing, (uint8_t status));
