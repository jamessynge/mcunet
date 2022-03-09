#include "tcp_server_connection.h"

#include <McuCore.h>

namespace mcunet {

MillisT ElapsedMillis(MillisT start_time) { return millis() - start_time; }

void DisconnectData::RecordDisconnect() {
  if (!disconnected) {
    MCU_VLOG(2) << MCU_FLASHSTR("DisconnectData::RecordDisconnect");
    disconnected = true;
    disconnect_time_millis = millis();
  }
}

void DisconnectData::Reset() {
  disconnected = false;
  disconnect_time_millis = 0;
}

MillisT DisconnectData::ElapsedDisconnectTime() {
  MCU_DCHECK(disconnected);
  return ElapsedMillis(disconnect_time_millis);
}

TcpServerConnection::TcpServerConnection(uint8_t *write_buffer,
                                         uint8_t write_buffer_limit,
                                         EthernetClient &client,
                                         DisconnectData &disconnect_data)
    : WriteBufferedWrappedClientConnection(write_buffer, write_buffer_limit),
      client_(client),
      disconnect_data_(disconnect_data) {
  MCU_VLOG(5) << MCU_FLASHSTR("TcpServerConnection@") << this
              << MCU_FLASHSTR(" ctor");
  disconnect_data_.Reset();
}
TcpServerConnection::~TcpServerConnection() {  // NOLINT
  MCU_VLOG(5) << MCU_FLASHSTR("TcpServerConnection@") << this
              << MCU_FLASHSTR(" dtor");
  flush();
}

void TcpServerConnection::close() {
  // The Ethernet5500 library's EthernetClient::stop method bakes in a limit
  // of 1 second for closing a connection, and spins in a loop waiting until
  // the connection closed, with a delay of 1 millisecond per loop. We avoid
  // this here by NOT delegating to the stop method. Instead we start the
  // close with a DISCONNECT operation (i.e. sending a FIN packet to the
  // peer).
  // TODO(jamessynge): Now that I've forked Ethernet3 'permanently' as
  // Ethernet5500, think about how to fix the issues with stop.
  auto socket_number = sock_num();
  auto status = PlatformEthernet::SocketStatus(socket_number);
  MCU_VLOG(2) << MCU_FLASHSTR("TcpServerConnection::close, sock_num=")
              << socket_number << MCU_FLASHSTR(", status=") << mcucore::BaseHex
              << status;
  if (status == SnSR::ESTABLISHED || status == SnSR::CLOSE_WAIT) {
    // We have an open connection. Make sure that any data in the write buffer
    // is sent.
    flush();
    status = PlatformEthernet::SocketStatus(socket_number);
    if (status == SnSR::ESTABLISHED || status == SnSR::CLOSE_WAIT) {
      PlatformEthernet::DisconnectSocket(socket_number);
      status = PlatformEthernet::SocketStatus(socket_number);
    }
  }
  // On the assumption that this is only called when there is a working
  // connection at the start of a call to the listener (e.g. OnHalfClosed), we
  // record this as a disconnect initiated by the listener so that we don't
  // later notify the listener of a disconnect
  disconnect_data_.RecordDisconnect();
}

bool TcpServerConnection::connected() const { return client_.connected(); }

bool TcpServerConnection::peer_half_closed() const {
  return PlatformEthernet::StatusIsHalfOpen(sock_num());
}

uint8_t TcpServerConnection::sock_num() const {
  return client_.getSocketNumber();
}

}  // namespace mcunet
