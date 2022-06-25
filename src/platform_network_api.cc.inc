////////////////////////////////////////////////////////////////////////////////
// Methods modifying sockets.

// Set socket 'sock_num' to listen for new TCP connections on port 'tcp_port',
// regardless of what that socket is doing now. Returns true if able to do so;
// false if not (e.g. if sock_num or tcp_port is invalid).
MCUNET_PNAPI_METHOD(bool, InitializeTcpListenerSocket,
                    (uint8_t sock_num, uint16_t tcp_port));

// Initiates a DISCONNECT of a TCP socket.
MCUNET_PNAPI_METHOD(bool, DisconnectSocket, (uint8_t sock_num));

// Forces a socket to be closed, with no packets sent out.
MCUNET_PNAPI_METHOD(bool, CloseSocket, (uint8_t sock_num));

////////////////////////////////////////////////////////////////////////////////
// Methods getting the status of a socket.

// Finds a hardware socket that is closed, and returns its socket number.
// Returns -1 if there is no such socket.
MCUNET_PNAPI_METHOD(int, FindUnusedSocket, ());

// Returns the implementation defined status value for the specified socket.
// TODO(jamessynge): Try to eliminate this method, and the methods such as
// StatusIsOpen below, with the aim of not exposing application code to
// hardware/implementation specific status types and values.
MCUNET_PNAPI_METHOD(uint8_t, SocketStatus, (uint8_t sock_num));

// Returns true if the hardware socket is being used for TCP and is not
// LISTENING; if so, then it is best not to repurpose the hardware socket.
MCUNET_PNAPI_METHOD(bool, SocketIsInTcpConnectionLifecycle, (uint8_t sock_num));

// Returns true if the hardware socket is listening for TCP connections.
MCUNET_PNAPI_METHOD(bool, SocketIsTcpListener,
                    (uint8_t sock_num, uint16_t tcp_port));

// Returns true if the socket is completely closed (not in use for any purpose).
MCUNET_PNAPI_METHOD(bool, SocketIsClosed, (uint8_t sock_num));

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
// TODO(jamessynge): Rename this to StatusIsHalfClosed or StatusPeerHasClosed;
// half-open refers to a state during the 3-way handshake when opening a
// connection, but this is intended to cover the case where the peer has half
// closed the connection, indicating that they won't write any more data.
MCUNET_PNAPI_METHOD(bool, StatusIsHalfOpen, (uint8_t status));

// Returns true if the status indicates that the TCP connection is in the
// process of closing (e.g. FIN_WAIT).
MCUNET_PNAPI_METHOD(bool, StatusIsClosing, (uint8_t status));
