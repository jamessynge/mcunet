#ifndef MCUNET_SRC_PLATFORM_NETWORK_INTERFACE_H_
#define MCUNET_SRC_PLATFORM_NETWORK_INTERFACE_H_

// PlatformNetworkInterface is used to provide the non-embedded implementation
// of PlatformNetwork, allowing mocking for tests, and a full host networking
// implementation.

#include <McuCore.h>

#include "mcunet_config.h"

#if MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION

#include <memory>  // pragma: keep standard include

namespace mcunet {

class PlatformNetworkInterface {
 public:
  virtual ~PlatformNetworkInterface();

  // Set *platform_network as the current implementation. There must not be a
  // current implementation.
  static void SetImplementation(
      std::unique_ptr<PlatformNetworkInterface> platform_network);

  // Remove (delete) the current implementation, which must be set.
  static void RemoveImplementation();

  // Get the current implementation.
  static PlatformNetworkInterface* GetImplementation();
  static PlatformNetworkInterface* GetImplementationOrDie();

#ifdef MCUNET_PNAPI_METHOD
#error "MCUNET_PNAPI_METHOD should not be defined!!"
#endif

#define MCUNET_PNAPI_METHOD(TYPE, NAME, ARGS) virtual TYPE NAME ARGS = 0
#include "platform_network_api.cc.inc"
#undef MCUNET_PNAPI_METHOD
};

// Supports installing and removing an implementation of
// PlatformNetworkInterface, of which there must be only one at a time.
template <typename T>  // T must extend PlatformNetworkInterface.
class PlatformNetworkLifetime {
 public:
  // Sets *platform_network as the current implementation.
  explicit PlatformNetworkLifetime(std::unique_ptr<T> platform_network)
      : platform_network_(platform_network.get()) {
    MCU_CHECK_NE(platform_network_, nullptr);
    PlatformNetworkInterface::SetImplementation(std::move(platform_network));
  }

  // Removes (deletes) current implementation, which must match the expected
  // value.
  ~PlatformNetworkLifetime() {
    MCU_CHECK_EQ(platform_network_,
                 PlatformNetworkInterface::GetImplementation());
    PlatformNetworkInterface::RemoveImplementation();
  }

  // Returns current implementation, which must match the expected value.
  T* platform_network() {
    MCU_CHECK_EQ(platform_network_,
                 PlatformNetworkInterface::GetImplementation());
    return platform_network_;
  }

 private:
  T* const platform_network_;
};

}  // namespace mcunet

#endif  // MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
#endif  // MCUNET_SRC_PLATFORM_NETWORK_INTERFACE_H_
