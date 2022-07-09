#ifndef MCUNET_SRC_TCP_SERVER_CONNECTION_H_
#define MCUNET_SRC_TCP_SERVER_CONNECTION_H_

// TcpServerConnection wraps an EthernetClient to provide buffered writing, and
// tracking of whether and when (clock ticks) a disconnect occurred, allowing
// for closing a connection in a non-blocking fashion.
//
// Author: james.synge@gmail.com

#include <McuCore.h>

#include "disconnect_data.h"
#include "platform_network.h"
#include "write_buffered_connection.h"

namespace mcunet {

class TcpServerConnection : public WriteBufferedConnection {
 public:
  TcpServerConnection(uint8_t* write_buffer, uint8_t write_buffer_limit,
                      EthernetClient& client, DisconnectData& disconnect_data);

  // If connection is open, flushes it and disconnects the socket, which is
  // recorded in the DisconnectData. This is non-blocking.
  void close() override;

  // Delegates to the wrapped client.
  uint8_t sock_num() const final { return sock_num_; }

 private:
  DisconnectData& disconnect_data_;
  const uint8_t sock_num_;
};

}  // namespace mcunet

#endif  // MCUNET_SRC_TCP_SERVER_CONNECTION_H_
