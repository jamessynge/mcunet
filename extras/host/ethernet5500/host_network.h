#ifndef MCUNET_EXTRAS_HOST_ETHERNET5500_HOST_NETWORK_H_
#define MCUNET_EXTRAS_HOST_ETHERNET5500_HOST_NETWORK_H_

// TODO(jamessynge): Describe why this file exists/what it provides.

#include <memory>

#include "platform_network.h"
#include "platform_network_interface.h"

namespace mcunet_host {
struct HostNetworkImpl;

class HostNetwork : public mcunet::PlatformNetworkInterface {
 public:
  HostNetwork();
  HostNetwork(const HostNetwork&) = delete;
  HostNetwork(HostNetwork&&) = delete;
  ~HostNetwork() override;

  HostNetwork& operator=(const HostNetwork&) = delete;
  HostNetwork& operator=(HostNetwork&&) = delete;

#ifdef MCUNET_PNAPI_METHOD
#error "MCUNET_PNAPI_METHOD should not be defined!!"
#endif

#define MCUNET_PNAPI_METHOD(TYPE, NAME, ARGS) TYPE NAME ARGS override
#include "platform_network_api.cc.inc"
#undef MCUNET_PNAPI_METHOD

  // Returns a TCP port that is free at the time of the call, for the purpose of
  // choosing a TCP server port for testing. Note that there is the possibility
  // of a race if another thread or process also needs a free port at the almost
  // the same time that this method is called: it is possible that both will be
  // told that the same port is free.
  static int FindFreeTcpPort();

 private:
  const std::unique_ptr<HostNetworkImpl> impl_;
};

}  // namespace mcunet_host

#endif  // MCUNET_EXTRAS_HOST_ETHERNET5500_HOST_NETWORK_H_