#ifndef MCUNET_SRC_PLATFORM_NETWORK_INTERFACE_H_
#define MCUNET_SRC_PLATFORM_NETWORK_INTERFACE_H_

// TODO(jamessynge): Describe why this file exists/what it provides.

#include <McuCore.h>

namespace mcunet {

class PlatformNetworkInterface {
 public:
  PlatformNetworkInterface();
  virtual ~PlatformNetworkInterface();

#ifdef MCUNET_PNAPI_METHOD
#error "MCUNET_PNAPI_METHOD should not be defined!!"
#endif

#define MCUNET_PNAPI_METHOD(TYPE, NAME, ARGS) virtual TYPE NAME ARGS = 0
#include "platform_network_api.cc.inc"
#undef MCUNET_PNAPI_METHOD
};

}  // namespace mcunet

#endif  // MCUNET_SRC_PLATFORM_NETWORK_INTERFACE_H_
