// Verify that the host impl of Ethernet5500 is able to do the basics.

#include <memory>

#include "absl/flags/flag.h"
#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "base/init_google.h"
#include "extras/host/ethernet5500/ethernet_config.h"
#include "extras/host/ethernet5500/host_network.h"
#include "platform_network_interface.h"

using ::mcunet::PlatformNetworkLifetime;
using ::mcunet_host::HostNetwork;

ABSL_FLAG(int, port, 0, "Port to listen on.");

int main(int argc, char* argv[]) {
  InitGoogle(argv[0], &argc, &argv, /*remove_flags=*/true);

  PlatformNetworkLifetime<HostNetwork> holder(std::make_unique<HostNetwork>());

  int port = absl::GetFlag(FLAGS_port);
  if (port <= 0) {
    port = HostNetwork::FindFreeTcpPort();
    LOG(INFO) << "Port " << port << " is free for TCP at this time.";
  }
  QCHECK_GT(port, 0);
  QCHECK_LT(port, 65536);

  {
    EthernetServer listener(port);
    listener.begin();
  }

  const auto start_time = absl::Now();
  const auto end_time = start_time + absl::Seconds(60);
  const auto interval = absl::Seconds(15);
  auto next_time = start_time + interval;
  bool connected = false;
  absl::Time now;
  while ((now = absl::Now()) < end_time && !connected) {
    while (now < next_time) {
      LOG(INFO) << "Waiting for a connection to port " << port;
      absl::SleepFor(next_time - now);
      now = absl::Now();
    }
    next_time += interval;

    EthernetServer listener(port);
    EthernetClient client = listener.available();
    if (client.getSocketNumber() >= MAX_SOCK_NUM) {
      LOG(INFO) << "No available connections.";
      continue;
    }
    connected = true;
    LOG(INFO) << "Accepted a connection on socket "
              << (client.getSocketNumber() + 0);
  }
  return connected ? 0 : 1;
}
