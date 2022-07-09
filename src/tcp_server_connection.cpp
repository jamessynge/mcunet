#include "tcp_server_connection.h"

#include <McuCore.h>

namespace mcunet {

TcpServerConnection::TcpServerConnection(uint8_t* write_buffer,
                                         uint8_t write_buffer_limit,
                                         EthernetClient& client,
                                         DisconnectData& disconnect_data)
    : WriteBufferedConnection(write_buffer, write_buffer_limit, client),
      disconnect_data_(disconnect_data),
      sock_num_(client.getSocketNumber()) {
  MCU_VLOG(5) << MCU_FLASHSTR("TcpServerConnection@") << this
              << MCU_FLASHSTR(" ctor");
  disconnect_data_.Reset();
}

void TcpServerConnection::close() {
  // The Ethernet5500 library's EthernetClient::stop method bakes in a limit
  // of 1 second for closing a connection, and spins in a loop waiting until
  // the connection closed, with a delay of 1 millisecond per loop. We avoid
  // this here by NOT delegating to the stop method. Instead we start the
  // close with a DISCONNECT operation (i.e. sending a FIN packet to the
  // peer).
  // TODO(jamessynge): Now that I've forked Ethernet3 'permanently' as
  // Ethernet5500, I need to think about how to fix the issues with stop.

  auto socket_number = sock_num();
  auto status = PlatformNetwork::SocketStatus(sock_num_);
  MCU_VLOG(2) << MCU_FLASHSTR("TcpServerConnection::close, sock_num=")
              << sock_num_ << MCU_FLASHSTR(", status=") << mcucore::BaseHex
              << status;
  if (status == SnSR::ESTABLISHED || status == SnSR::CLOSE_WAIT) {
    // We have an open connection. Make sure that any data in the write buffer
    // is sent.
    flush();
    status = PlatformNetwork::SocketStatus(sock_num_);
    if (status == SnSR::ESTABLISHED || status == SnSR::CLOSE_WAIT) {
      PlatformNetwork::DisconnectSocket(socket_number);
      status = PlatformNetwork::SocketStatus(socket_number);
    }
  }
  // On the assumption that this is only called when there is a working
  // connection at the start of a call to the listener (e.g. OnHalfClosed), we
  // record this as a disconnect initiated by the listener so that we don't
  // later notify the listener of a disconnect
  disconnect_data_.RecordDisconnect();
}

}  // namespace mcunet
