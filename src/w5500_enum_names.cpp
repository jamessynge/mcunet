#include "w5500_enum_names.h"

#include <McuCore.h>

#include "platform_network.h"

namespace mcunet {

size_t PrintValueTo(const SnSRName value, Print& out) {
  // Not using a switch statement because the avr-gcc compiler generates a jump
  // table in RAM.
  if (value.value == SnSR::CLOSED) {
    return out.print(MCU_FLASHSTR("CLOSED"));
  }
  if (value.value == SnSR::INIT) {
    return out.print(MCU_FLASHSTR("INIT"));
  }
  if (value.value == SnSR::LISTEN) {
    return out.print(MCU_FLASHSTR("LISTEN"));
  }
  if (value.value == SnSR::SYNSENT) {
    return out.print(MCU_FLASHSTR("SYNSENT"));
  }
  if (value.value == SnSR::SYNRECV) {
    return out.print(MCU_FLASHSTR("SYNRECV"));
  }
  if (value.value == SnSR::ESTABLISHED) {
    return out.print(MCU_FLASHSTR("ESTABLISHED"));
  }
  if (value.value == SnSR::FIN_WAIT) {
    return out.print(MCU_FLASHSTR("FIN_WAIT"));
  }
  if (value.value == SnSR::CLOSING) {
    return out.print(MCU_FLASHSTR("CLOSING"));
  }
  if (value.value == SnSR::TIME_WAIT) {
    return out.print(MCU_FLASHSTR("TIME_WAIT"));
  }
  if (value.value == SnSR::CLOSE_WAIT) {
    return out.print(MCU_FLASHSTR("CLOSE_WAIT"));
  }
  if (value.value == SnSR::LAST_ACK) {
    return out.print(MCU_FLASHSTR("LAST_ACK"));
  }
  if (value.value == SnSR::UDP) {
    return out.print(MCU_FLASHSTR("UDP"));
  }
  if (value.value == SnSR::IPRAW) {
    return out.print(MCU_FLASHSTR("IPRAW"));
  }
  if (value.value == SnSR::MACRAW) {
    return out.print(MCU_FLASHSTR("MACRAW"));
  }
  if (value.value == SnSR::PPPOE) {
    return out.print(MCU_FLASHSTR("PPPOE"));
  }
  size_t result = out.print('0');
  result += out.print('x');
  result += out.print(0 + value.value, HEX);
  return result;
}

}  // namespace mcunet
