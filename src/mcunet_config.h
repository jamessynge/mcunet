#ifndef MCUNET_SRC_MCUNET_CONFIG_H_
#define MCUNET_SRC_MCUNET_CONFIG_H_

// Configure McuNet features.
//
// Author: james.synge@gmail.com

#include <McuCore.h>

#ifndef MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION
#if MCU_HOST_TARGET
#define MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION 1
#endif  // MCU_HOST_TARGET
#endif  // MCU_HAS_PLATFORM_NETWORK_IMPLEMENTATION

#endif  // MCUNET_SRC_MCUNET_CONFIG_H_
