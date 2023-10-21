#ifndef MCUNET_EXTRAS_HOST_ETHERNET5500_ETHERNET5500_H_
#define MCUNET_EXTRAS_HOST_ETHERNET5500_ETHERNET5500_H_

// Just enough of Ethernet5500's classes for Tiny Alpaca Server to compile on
// host, maybe to be a TCP & UDP server.
//
// Author: james.synge@gmail.com

#include "extras/host/ethernet5500/dhcp_class.h"       // IWYU pragma: export
#include "extras/host/ethernet5500/ethernet_class.h"   // IWYU pragma: export
#include "extras/host/ethernet5500/ethernet_client.h"  // IWYU pragma: export
#include "extras/host/ethernet5500/ethernet_config.h"  // IWYU pragma: export
#include "extras/host/ethernet5500/ethernet_server.h"  // IWYU pragma: export
#include "extras/host/ethernet5500/ethernet_udp.h"     // IWYU pragma: export
#include "extras/host/ethernet5500/w5500.h"
#include "mcucore/extras/host/arduino/ip_address.h"  // IWYU pragma: export

#endif  // MCUNET_EXTRAS_HOST_ETHERNET5500_ETHERNET5500_H_
