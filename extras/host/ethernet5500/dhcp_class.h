#ifndef MCUNET_EXTRAS_HOST_ETHERNET5500_DHCP_CLASS_H_
#define MCUNET_EXTRAS_HOST_ETHERNET5500_DHCP_CLASS_H_

// Incomplete declaration/implementation of Ethernet5500's DhcpClass. It is used
// in the embedded environment to lease an IP address for the device, etc. On
// the host it does nothing.
//
// Author: james.synge@gmail.com

#define DHCP_CHECK_NONE (0)
#define DHCP_CHECK_RENEW_FAIL (1)
#define DHCP_CHECK_RENEW_OK (2)
#define DHCP_CHECK_REBIND_FAIL (3)
#define DHCP_CHECK_REBIND_OK (4)

class DhcpClass {};

#endif  // MCUNET_EXTRAS_HOST_ETHERNET5500_DHCP_CLASS_H_
