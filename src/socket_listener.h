#ifndef MCUNET_SRC_SOCKET_LISTENER_H_
#define MCUNET_SRC_SOCKET_LISTENER_H_

// The APIs for notifying listeners of key socket events.
//
// Author: james.synge@gmail.com

#include <McuCore.h>

#include "connection.h"

namespace mcunet {

// There is no OnCanWrite because we don't seem to need it (for now) for the
// purposes of Tiny Alpaca Server, i.e. where we don't have long streams of data
// to return and don't have the ability to buffer data and resume writing later
// when there is room for more.
class SocketListener {
 public:
#if !MCU_EMBEDDED_TARGET
  virtual ~SocketListener() = default;
#endif

  // Called when there *may* be data to read from the peer. Currently this is
  // also called by ServerSocket for existing connections.
  // TODO(jamessynge): Consider renaming to OnPerformIo.
  virtual void OnCanRead(Connection& connection) = 0;

  // Called when we discover that the connection has been fully or partially
  // closed, typically by the other party; however, this might also occur if
  // network configuration needs to be reset, such as when the DHCP lease has
  // expired.
  virtual void OnDisconnect() = 0;
};

class ServerSocketListener : public SocketListener {
 public:
  // Called when a new connection from a client is received.
  virtual void OnConnect(Connection& connection) = 0;
};

}  // namespace mcunet

#endif  // MCUNET_SRC_SOCKET_LISTENER_H_
