#ifndef MCUNET_SRC_DISCONNECT_DATA_H_
#define MCUNET_SRC_DISCONNECT_DATA_H_

// DisconnectData is used to whether and when when we detected or initiated the
// close of a connection.

#include <McuCore.h>

namespace mcunet {

// Returns the milliseconds since start_time. Beware of wrap around.
mcucore::MillisT ElapsedMillis(mcucore::MillisT start_time);

struct DisconnectData {
  // Mark as NOT disconnected.
  void Reset();

  // Mark as disconnected, if not already marked as such, and if so record the
  // current time.
  void RecordDisconnect();

  // Time since RecordDisconnect set disconnected and disconnect_time_millis.
  mcucore::MillisT ElapsedDisconnectTime();

  // True if disconnected, false otherwise. Starts disconnected, i.e. we don't
  // have a connection at startup.
  bool disconnected = true;

  // Time at which RecordDisconnect recorded a disconnect (i.e. the first such
  // call after Reset()).
  mcucore::MillisT disconnect_time_millis = 0;
};

}  // namespace mcunet

#endif  // MCUNET_SRC_DISCONNECT_DATA_H_
