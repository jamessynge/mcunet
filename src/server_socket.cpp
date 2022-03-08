#include "server_socket.h"

#include <McuCore.h>

#include "connection.h"
#include "platform_ethernet.h"

namespace mcunet {
namespace {

constexpr MillisT kDisconnectMaxMillis = 5000;
constexpr uint8_t kWriteBufferSize = 255;

}  // namespace

ServerSocket::ServerSocket(uint16_t tcp_port, ServerSocketListener &listener)
    : sock_num_(MAX_SOCK_NUM),
      last_status_(SnSR::CLOSED),
      listener_(listener),
      tcp_port_(tcp_port) {}

bool ServerSocket::PickClosedSocket() {
  if (HasSocket()) {
    return false;
  }

  last_status_ = SnSR::CLOSED;

  int sock_num = PlatformEthernet::FindUnusedSocket();
  if (0 <= sock_num && sock_num < MAX_SOCK_NUM) {
    sock_num_ = sock_num & 0xff;
    if (BeginListening()) {
      last_status_ = PlatformEthernet::SocketStatus(sock_num_);
      return true;
    }
    MCU_VLOG(1) << MCU_FLASHSTR("listen for ") << tcp_port_
                << MCU_FLASHSTR(" failed with socket ") << sock_num_;
    sock_num_ = MAX_SOCK_NUM;
  } else {
    MCU_VLOG(1) << MCU_FLASHSTR("No free socket for ") << tcp_port_;
  }
  return false;
}

bool ServerSocket::HasSocket() const { return sock_num_ < MAX_SOCK_NUM; }

bool ServerSocket::IsConnected() const {
  return HasSocket() &&
         PlatformEthernet::SocketIsInTcpConnectionLifecycle(sock_num_);
}

bool ServerSocket::ReleaseSocket() {
  MCU_DCHECK(HasSocket());
  if (IsConnected()) {
    return false;
  }

  CloseHardwareSocket();
  sock_num_ = 0;
  return true;
}

#define STATUS_IS_UNEXPECTED_MESSAGE(expected_str, some_status,               \
                                     current_status)                          \
  mcucore::BaseHex << MCU_FLASHSTR("Expected " #some_status " to be ")        \
                   << MCU_FLASHSTR(expected_str) << MCU_FLASHSTR(", but is ") \
                   << mcucore::BaseHex << some_status                         \
                   << MCU_FLASHSTR("; current status is ") << current_status

#define VERIFY_STATUS_IS(expected_status, some_status)                        \
  MCU_DCHECK_EQ(expected_status, some_status)                                 \
      << mcucore::BaseHex << MCU_FLASHSTR("Expected " #some_status " to be ") \
      << expected_status << MCU_FLASHSTR(", but is ") << some_status

bool ServerSocket::BeginListening() {
  if (!HasSocket()) {
    return false;
  } else if (PlatformEthernet::SocketIsTcpListener(sock_num_, tcp_port_)) {
    // Already listening.
    MCU_VLOG(1) << MCU_FLASHSTR("Already listening, last_status_ is ")
                << mcucore::BaseHex << last_status_;
    last_status_ = PlatformEthernet::SocketStatus(sock_num_);
    return true;
  } else if (IsConnected()) {
    return false;
  }

  CloseHardwareSocket();

  if (PlatformEthernet::InitializeTcpListenerSocket(sock_num_, tcp_port_)) {
    last_status_ = PlatformEthernet::SocketStatus(sock_num_);
    MCU_VLOG(1) << MCU_FLASHSTR("Listening to port ") << tcp_port_
                << MCU_FLASHSTR(" on socket ") << sock_num_
                << MCU_FLASHSTR(", last_status is ") << mcucore::BaseHex
                << last_status_;
    VERIFY_STATUS_IS(SnSR::LISTEN, last_status_);
    return true;
  }
  MCU_VLOG(1) << MCU_FLASHSTR("listen for ") << tcp_port_
              << MCU_FLASHSTR(" failed with socket ") << sock_num_;
  return false;
}

// Notifies listener_ of relevant events/states of the socket (i.e. a new
// connection from a client, available data to read, room to write, client
// disconnect). The current implementation will make at most one of the
// On<Event> calls per call to PerformIO. This method is expected to be called
// from the loop() function of an Arduino sketch (i.e. typically hundreds or
// thousands of times a second).
// TODO(jamessynge): IFF this doesn't perform well enough, investigate using the
// interrupt features of the W5500 to learn which sockets have state changes
// more rapidly.
void ServerSocket::PerformIO() {
  if (!HasSocket()) {
    return;
  }
  const auto status = PlatformEthernet::SocketStatus(sock_num_);
  const bool is_open = PlatformEthernet::StatusIsOpen(status);
  const auto past_status = last_status_;
  const bool was_open = PlatformEthernet::StatusIsOpen(past_status);

  last_status_ = status;

  if (was_open && !is_open) {
    // Connection closed without us taking action. Let the listener know.
    MCU_VLOG(2) << MCU_FLASHSTR("was open, not now");
    if (!disconnect_data_.disconnected) {
      disconnect_data_.RecordDisconnect();
      listener_.OnDisconnect();
    } else {
      MCU_VLOG(2) << MCU_FLASHSTR("Disconnect already recorded");
    }
    // We'll deal with the new status next time (e.g. FIN_WAIT or closing)
    return;
  }

  switch (status) {
    case SnSR::CLOSED:
      BeginListening();
      break;

    case SnSR::LISTEN:
      VERIFY_STATUS_IS(SnSR::LISTEN, past_status);
      break;

    case SnSR::SYNRECV:
      // This is a transient state that the chip handles (i.e. responds with a
      // SYN/ACK, waits for an ACK from the client to complete the three step
      // TCP handshake). If that times out or a RST is recieved, then the W5500
      // socket will transition to to closed, and we'll have to call
      // BeginListening again.
      //
      // To keep the debug macros in the following states simple, we overwrite
      // last_status_ here.
      VERIFY_STATUS_IS(SnSR::LISTEN, past_status);
      last_status_ = SnSR::LISTEN;
      break;

    case SnSR::ESTABLISHED:
      if (!was_open) {
        VERIFY_STATUS_IS(SnSR::LISTEN, past_status)
            << MCU_FLASHSTR(" while handling ESTABLISHED");
        AnnounceConnected();
      } else {
        VERIFY_STATUS_IS(SnSR::ESTABLISHED, past_status)
            << MCU_FLASHSTR(" while handling ESTABLISHED");
        AnnounceCanRead();
      }
      break;

    case SnSR::CLOSE_WAIT:
      if (!was_open) {
        VERIFY_STATUS_IS(SnSR::LISTEN, past_status)
            << MCU_FLASHSTR(" while handling CLOSE_WAIT");
        AnnounceConnected();
      } else {
        MCU_DCHECK(was_open) << STATUS_IS_UNEXPECTED_MESSAGE(
            "ESTABLISHED or CLOSE_WAIT", past_status, status);
        HandleCloseWait();
      }
      break;

    case SnSR::FIN_WAIT:
    case SnSR::CLOSING:
    case SnSR::TIME_WAIT:
    case SnSR::LAST_ACK:
      // Transient states after the connection is closed, but before the final
      // cleanup is complete.
      MCU_DCHECK(was_open || PlatformEthernet::StatusIsClosing(past_status))
          << STATUS_IS_UNEXPECTED_MESSAGE("a closing value", past_status,
                                          status);

      DetectCloseTimeout();
      break;

    case SnSR::INIT:
      // This is a transient state during setup of a TCP listener, and should
      // not be visible to us because BeginListening should make calls that
      // complete the process.
      MCU_DCHECK(false) << MCU_FLASHSTR(
                               "Socket in INIT state, incomplete LISTEN setup; "
                               "past_status is ")
                        << past_status;
      if (past_status == SnSR::INIT) {
        // Apparently stuck in this state.
        CloseHardwareSocket();
      }
      break;

    ///////////////////////////////////////////////////////////////////////////
    // States that the hardware socket should not be in if it is being used as a
    // TCP server socket.
    case SnSR::SYNSENT:
      // SYNSENT indicates we decided to use the socket as a client socket. Must
      // release the socket first.

    case SnSR::UDP:
    case SnSR::IPRAW:
    case SnSR::MACRAW:
    case SnSR::PPPOE:
      MCU_DCHECK(false) << MCU_FLASHSTR("Socket ") << sock_num_
                        << mcucore::BaseHex
                        << MCU_FLASHSTR(" has unexpected status ") << status
                        << MCU_FLASHSTR(", past_status is ") << past_status;
      CloseHardwareSocket();
      break;

    default:
      // I noticed that status sometimes equals 0x11 after LISTEN, but 0x11 is
      // not a documented value. I asked on the WIZnet developer forum, and got
      // this response:
      //
      //   You can ignore it except for the state values specified in the
      //   datasheet. All status values are not disclosed due to company policy.
      //
      // Therefore, I'm restoring last_status_ to the value it had prior to this
      // undocumented status. Note that this works in this spot, but there are
      // other areas in the code where it will cause a problem. Sigh.
      last_status_ = past_status;
      break;
  }
}

// NOTE: Could choose to add another method that accepts a member function
// pointer to one of the SocketListener methods, and then delegate from the
// AnnounceX methods to that method. It may be worth doing if it notably reduces
// flash consumption.

void ServerSocket::AnnounceConnected() {
  EthernetClient client(sock_num_);
  uint8_t write_buffer[kWriteBufferSize];
  TcpServerConnection conn(write_buffer, kWriteBufferSize, client,
                           disconnect_data_);
  listener_.OnConnect(conn);
  DetectListenerInitiatedDisconnect();
}

void ServerSocket::AnnounceCanRead() {
  EthernetClient client(sock_num_);
  uint8_t write_buffer[kWriteBufferSize];
  TcpServerConnection conn(write_buffer, kWriteBufferSize, client,
                           disconnect_data_);
  listener_.OnCanRead(conn);
  DetectListenerInitiatedDisconnect();
}

void ServerSocket::HandleCloseWait() {
  EthernetClient client(sock_num_);
  uint8_t write_buffer[kWriteBufferSize];
  TcpServerConnection conn(write_buffer, kWriteBufferSize, client,
                           disconnect_data_);
  if (client.available() > 0) {
    // Still have data that we can read from the client (i.e. buffered up in the
    // network chip).
    // TODO(jamessynge): Determine whether we get the CLOSE_WAIT state before
    // we've read all the data from the client, or only once we've drained those
    // buffers.
    listener_.OnCanRead(conn);
  } else {
    listener_.OnHalfClosed(conn);
    MCU_VLOG(2) << MCU_FLASHSTR("HandleCloseWait ")
                << MCU_FLASHSTR("disconnected=")
                << disconnect_data_.disconnected;
  }
  DetectListenerInitiatedDisconnect();
}

void ServerSocket::DetectListenerInitiatedDisconnect() {
  MCU_VLOG(9) << MCU_FLASHSTR("DetectListenerInitiatedDisconnect ")
              << MCU_FLASHSTR("disconnected=") << disconnect_data_.disconnected;
  if (disconnect_data_.disconnected) {
    auto new_status = PlatformEthernet::SocketStatus(sock_num_);
    MCU_VLOG(2) << MCU_FLASHSTR("DetectListenerInitiatedDisconnect")
                << mcucore::BaseHex << MCU_FLASHSTR(" last_status=")
                << last_status_ << MCU_FLASHSTR(" new_status=") << new_status;
    last_status_ = new_status;
  }
}

void ServerSocket::DetectCloseTimeout() {
  // The Ethernet3 library baked in a limit of 1 second for closing a
  // connection, and did so by using a loop checking to see if the connection
  // closed. Since this implementation doesn't block in a loop, we can allow a
  // bit more time.
  if (disconnect_data_.disconnected &&
      disconnect_data_.ElapsedDisconnectTime() > kDisconnectMaxMillis) {
    // Time to give up.
    MCU_VLOG(2) << MCU_FLASHSTR("DetectCloseTimeout closing socket");
    CloseHardwareSocket();
  }
}

void ServerSocket::CloseHardwareSocket() {
  MCU_VLOG(2) << MCU_FLASHSTR("CloseHardwareSocket")
              << MCU_FLASHSTR(" last_status=") << mcucore::BaseHex
              << last_status_;
  PlatformEthernet::CloseSocket(sock_num_);
  last_status_ = PlatformEthernet::SocketStatus(sock_num_);
  MCU_DCHECK_EQ(last_status_, SnSR::CLOSED);
}

}  // namespace mcunet
