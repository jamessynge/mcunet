#ifndef MCUNET_SRC_PLATFORM_NETWORK_H_
#define MCUNET_SRC_PLATFORM_NETWORK_H_

// PlatformNetwork provides static methods for use by network server and client
// code to manage sockets. Once a socket is setup, EthernetClient and
// EthernetUdp classes can be used to perform I/O on those sockets.
//
// Author: james.synge@gmail.com

#include <McuCore.h>  // IWYU pragma: export

#include "mcunet_config.h"
#include "platform_network_interface.h"

#ifdef ARDUINO

#include <Client.h>          // IWYU pragma: export
#include <Ethernet5500.h>    // IWYU pragma: export
#include <IPAddress.h>       // IWYU pragma: export
#include <Stream.h>          // IWYU pragma: export
#include <utility/socket.h>  // IWYU pragma: export

#elif MCU_EMBEDDED_TARGET

#error "No support known for this platform."

#elif MCU_HOST_TARGET

// These are included here to avoid needing to include them explicitly
// elsewhere, but it is a bit excessive. As I improve the
// implementation-independent networking API, I hope to reduce the number of
// these includes.
#include "extras/host/arduino/client.h"             // IWYU pragma: export
#include "extras/host/arduino/ip_address.h"         // IWYU pragma: export
#include "extras/host/ethernet5500/ethernet5500.h"  // IWYU pragma: export
#include "extras/host/ethernet5500/host_sockets.h"  // IWYU pragma: export

#endif  // ARDUINO

namespace mcunet {

struct PlatformNetwork {
#ifdef MCUNET_PNAPI_METHOD
#error "MCUNET_PNAPI_METHOD should not be defined!!"
#endif

#define MCUNET_PNAPI_METHOD(TYPE, NAME, ARGS) static TYPE NAME ARGS
#include "platform_network_api.cc.inc"
#undef MCUNET_PNAPI_METHOD
};

}  // namespace mcunet

#endif  // MCUNET_SRC_PLATFORM_NETWORK_H_
