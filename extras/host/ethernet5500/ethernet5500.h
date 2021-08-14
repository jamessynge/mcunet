#ifndef MCUNET_EXTRAS_HOST_ETHERNET5500_ETHERNET5500_H_
#define MCUNET_EXTRAS_HOST_ETHERNET5500_ETHERNET5500_H_

// Just enough of Ethernet5500's classes for Tiny Alpaca Server to compile on
// host, maybe to be a TCP & UDP server.

#include <IPAddress.h>  // IWYU pragma: export

#include "extras/host/ethernet5500/dhcp_class.h"       // IWYU pragma: export
#include "extras/host/ethernet5500/ethernet_class.h"   // IWYU pragma: export
#include "extras/host/ethernet5500/ethernet_client.h"  // IWYU pragma: export
#include "extras/host/ethernet5500/ethernet_config.h"  // IWYU pragma: export
#include "extras/host/ethernet5500/ethernet_server.h"  // IWYU pragma: export
#include "extras/host/ethernet5500/ethernet_udp.h"     // IWYU pragma: export
#include "extras/host/ethernet5500/w5500.h"

#endif  // MCUNET_EXTRAS_HOST_ETHERNET5500_ETHERNET5500_H_
