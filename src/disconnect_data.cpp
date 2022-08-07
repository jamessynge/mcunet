#include "disconnect_data.h"

#include <McuCore.h>

namespace mcunet {

void DisconnectData::RecordDisconnect() {
  if (!disconnected) {
    MCU_VLOG(2) << MCU_PSD("DisconnectData::RecordDisconnect");
    disconnected = true;
    disconnect_time_millis = millis();
  }
}

void DisconnectData::Reset() {
  disconnected = false;
  disconnect_time_millis = 0;
}

mcucore::MillisT DisconnectData::ElapsedDisconnectTime() {
  MCU_DCHECK(disconnected);
  return mcucore::ElapsedMillis(disconnect_time_millis);
}

}  // namespace mcunet
