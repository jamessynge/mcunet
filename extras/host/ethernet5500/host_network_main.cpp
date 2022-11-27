// Provides the minimal wrapper around setup() and loop()

#include <memory>

#include "base/init_google.h"
#include "extras/host/ethernet5500/host_network.h"
#include "mcucore/extras/host/arduino/call_setup_and_loop.h"
#include "platform_network_interface.h"

using ::mcunet_host::HostNetwork;

int main(int argc, char* argv[]) {
  InitGoogle(argv[0], &argc, &argv, /*remove_flags=*/true);
  mcunet::PlatformNetworkLifetime<HostNetwork> holder(
      std::make_unique<HostNetwork>());
  mcucore_host::CallSetupAndLoop();
  return 0;
}
