# Networking API

Various thoughts regarding the API for performing networking on a
microcontroller...

McuNet (and McuCore) build on the Arduino API in order to be applicable to a
broad range of devices, and for ease of use by a broad range of folks who are
likely to use the Arduino IDE, or maybe the Arduino CLI.

The API exposed by
[Ethernet5500_Arduino_Library](https://github.com/jamessynge/Ethernet5500_Arduino_Library)
(based on Ethernet3 and earlier Arduino networking libraries) makes a bunch of
assumptions about specific hardware, such as the WIZnet W5500 networking chip.
In particular, the idea that there are a specific, small number of hardware
sockets, each of which can be used for a single purpose at a time, such as for:

*   Listening to a TCP socket for a (single) new connection from a client (i.e.
    acting as a server waiting for a client);
*   Opening a TCP connection to a remote server (i.e. acting as a client);
*   Communicating over TCP with a single peer, reading from and writing to a
    byte stream;
*   Communicating over UDP with (potentially) multiple peers, including sending
    or receiving broadcast datagrams;
*   Reading arbitrary packets from the network (aka promiscuous mode, or MACRAW
    in WIZnet terminology).

A small amount of configuration data is required by Ethernet5500 for controlling
the W5500:

*   The chip select pin for SPI communication; implicitly this requires that no
    other SPI device is enabled at the same time, but (quite reasonably)
    Ethernet5500 isn't able to track that.
*   The hard reset pin; while required by Ethernet5500, this is *sort of*
    optional because the microcontroller may not actually have a pin hooked up
    to the reset pin of the W5500; on the RobotDyn Mega 2560 ETH board there is
    a solder bridge jumper which can be used to enable using a specific GPIO
    (D7) to reset the W5500.

In addition, the W5500 can emit an interrupt signal to the host microcontroller;
on the RobotDyn Mega 2560 ETH board there is a solder bridge jumper which can
enable connecting this output from the W5500 to pin D8 of the ATmega 2560. This
isn't a feature that Ethernet5500 makes use of.

## Questions

Some high-level questions:

*   Should I aim for something that is very close to the Posix Sockets API, or
    for an API that is more closely aligned to the WIZnet W5500 hardware API? It
    makes more sense to have the embedded socket API implementation be simple,
    than for the host implementation (i.e. an adapter to the Posix Socket API)
    to be simple. OTOH, the application code calling the API should also be
    relatively simple, and the core of the Posix Sockets API is pretty simple.

> My inclination is to use the Posix Sockets API as the basis, with as few
> extensions as necessary to support hardware specific features (e.g. specifying
> GPIO pin numbers), and to use compile-time selection of the hardware specific
> adapter (driver) code, such as for the W5500 chip. This might be aided by
> having types such as NetworkSocket and SocketStatus be provided by the driver,
> allowing for different numbers of required bits.
>
> This is best done by having a struct expose the API, including the Posix API,
> as static methods. This provides a separation from the very similar host or
> hardware specific APIs.

*   Should we require an application to ask specific questions, or provide a
    means to get callbacks, either as interrupts or by calling a polling API
    periodically, passing in a handler to be called with appropriate args
    indicating the current state.

> For the reasons described above for preferring the Posix Sockets API, I think
> sticking with polling is the better choice. That doesn't preclude exposing a
> means of getting interrupts, so long as it doesn't exclude using the standard
> inspired API.

## Requirements

*   Continue to allow the use of the Arduino Client class for I/O on open TCP
    connections. The implementation of that should not require much RAM (e.g.
    for socket tables). The W5500 implementation mostly delegates to either a
    sockets API library, or to W5500 specific methods.
*   Initialize the device driver (e.g. with GPIO pins; the amount of memory to
    be dedicated to each socket, which is a trade-off the W5500 provides w.r.t.
    the number of hardware sockets).
*   Reset a single socket or all sockets.
*   Determine the maximum and configured number of sockets.
*   Configuring a socket for a purpose (client, or server, TCP or UDP)
*   Opening a connection on a closed socket.
*   Gracefully closing an open TCP socket.
*   Forcefully closing (resetting) a socket (maybe the same as resetting it).
*   Read peer identity for a TCP socket with an open connection.
*   Read peer identity for a UDP socket while receiving a datagram.
*   Detect when a server socket has received a connection or datagram. (This may
    not be strictly necessary: we can just ask if there is data to read, and
    separately track whether there was any connection in progress before that.)
*   Read some bytes from a TCP socket into a buffer, with a response indicating
    how many were actually read into the buffer.
*   Write bytes in a buffer to a TCP socket, with a response indicating how many
    were actually written, or whether there was an error (i.e. the connection is
    closed or lost).
*   Read the bytes of a UDP datagram in chunks (on the assumption that we may
    not have sufficient space to store an entire datagram in one memory buffer).
*   Detect the boundaries between UDP datagrams while reading.
*   Write the bytes of a datagram in chunks (on the assumption that we may not
    have sufficient space to store an entire datagram in one memory buffer).
