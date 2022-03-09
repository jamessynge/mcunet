#ifndef MCUNET_SRC_TCP_SERVER_CONNECTION_H_
#define MCUNET_SRC_TCP_SERVER_CONNECTION_H_

// TcpServerConnection wraps an EthernetClient to provide buffered writing, and
// tracking of when a disconnect occurred, allowing for closing a connection in
// a non-blocking fashion.
//
// Author: james.synge@gmail.com

#include <McuCore.h>

#include "connection.h"
#include "socket_listener.h"

namespace mcunet {

// Returns the milliseconds since start_time. Beware of wrap around.
MillisT ElapsedMillis(MillisT start_time);

// Struct used to record when we detected or initiated the close of a
// connection.
struct DisconnectData {
  // Mark as NOT disconnected.
  void Reset();

  // Mark as disconnected, if not already marked as such, and if so record the
  // current time.
  void RecordDisconnect();

  // Time since RecordDisconnect set disconnected and disconnect_time_millis.
  MillisT ElapsedDisconnectTime();

  // True if disconnected, false otherwise. Starts disconnected, i.e. we don't
  // have a connection at startup.
  bool disconnected = true;

  // Time at which RecordDisconnect recorded a disconnect (i.e. the first such
  // call after Reset()).
  MillisT disconnect_time_millis = 0;
};

class TcpServerConnection : public WriteBufferedWrappedClientConnection {
 public:
  TcpServerConnection(uint8_t *write_buffer, uint8_t write_buffer_limit,
                      EthernetClient &client, DisconnectData &disconnect_data);
  ~TcpServerConnection() override;

  // Closes the connection and records the connection as disconnected. This is
  // non-blocking.
  void close() override;
  bool connected() const override;
  bool peer_half_closed() const override;
  uint8_t sock_num() const override;

 protected:
  Client &client() const override { return client_; }

 private:
  EthernetClient &client_;
  DisconnectData &disconnect_data_;
};

}  // namespace mcunet

#endif  // MCUNET_SRC_TCP_SERVER_CONNECTION_H_
