#include "platform_network_interface.h"

#if MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION

#include <McuCore.h>

#include <memory>   // pragma: keep standard include
#include <utility>  // pragma: keep standard include

namespace mcunet {
namespace {
std::unique_ptr<PlatformNetworkInterface> g_platform_network;  // NOLINT
}  // namespace

PlatformNetworkInterface::~PlatformNetworkInterface() {
  MCU_CHECK_NE(this, g_platform_network.get());
}

void PlatformNetworkInterface::SetImplementation(
    std::unique_ptr<PlatformNetworkInterface> platform_network) {
  MCU_CHECK_EQ(g_platform_network, nullptr);
  g_platform_network = std::move(platform_network);
}

void PlatformNetworkInterface::RemoveImplementation() {
  MCU_DCHECK_NE(g_platform_network, nullptr);
  g_platform_network.reset();
}

PlatformNetworkInterface* PlatformNetworkInterface::GetImplementation() {
  return g_platform_network.get();
}

PlatformNetworkInterface* PlatformNetworkInterface::GetImplementationOrDie() {
  MCU_DCHECK_NE(g_platform_network.get(), nullptr);
  return g_platform_network.get();
}

}  // namespace mcunet

#endif  // MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
