#ifndef MCUNET_SRC_TCP_SERVER_CONNECTION_H_
#define MCUNET_SRC_TCP_SERVER_CONNECTION_H_

// TcpServerConnection wraps an EthernetClient to provide buffered writing, and
// tracking of when a disconnect occurred.
//
// Author: james.synge@gmail.com

#include <McuCore.h>

#include "connection.h"
#include "socket_listener.h"

namespace mcunet {

// Returns the milliseconds since start_time. Beware of wrap around.
MillisT ElapsedMillis(MillisT start_time);

// Struct used to record whether the listener called Connection::close(), and
// if so, when.
struct DisconnectData {
  // Mark as NOT disconnected.
  void Reset();

  // Mark as disconnected, if not already disconnected.
  void RecordDisconnect();

  // Time since RecordDisconnect set disconnected and disconnect_time_millis.
  MillisT ElapsedDisconnectTime();

  // True if disconnected, false otherwise.
  bool disconnected;

  // Time at which RecordDisconnect recorded a disconnect (i.e. the first such
  // call after Reset()).
  MillisT disconnect_time_millis;
};

class TcpServerConnection : public WriteBufferedWrappedClientConnection {
 public:
  TcpServerConnection(uint8_t *write_buffer, uint8_t write_buffer_limit,
                      EthernetClient &client, DisconnectData &disconnect_data);
  ~TcpServerConnection() override;

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
