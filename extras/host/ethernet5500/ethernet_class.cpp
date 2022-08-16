#include "extras/host/ethernet5500/ethernet_class.h"

#include "absl/log/check.h"
#include "absl/log/log.h"
#include "extras/host/ethernet5500/ethernet_config.h"
#include "platform_network_interface.h"

EthernetClass Ethernet;  // NOLINT

// static
uint8_t EthernetClass::_state[MAX_SOCK_NUM] = {};
// static
uint16_t EthernetClass::_server_port[MAX_SOCK_NUM] = {};

namespace {
// Close all sockets, whether they're currently open or not.
void CloseAllSockets() {
  auto* const platform_network =
      mcunet::PlatformNetworkInterface::GetImplementationOrDie();
  for (uint8_t sock_num = 0; sock_num < MAX_SOCK_NUM; ++sock_num) {
    platform_network->CloseSocket(sock_num);
  }
}
}  // namespace

EthernetClass::EthernetClass()
    : _maxSockNum(MAX_SOCK_NUM), _pinCS(255), _pinRST(255), _dhcp(nullptr) {
  _customHostname[0] = 0;
}

void EthernetClass::init(uint8_t maxSockNum) {
  DCHECK_GE(maxSockNum, 1);
  DCHECK_LE(maxSockNum, 8);
}

uint8_t EthernetClass::softreset() {
  VLOG(3) << "EthernetClass::softreset entry";
  CHECK_GE(chip_select_pin_, 0);
  CloseAllSockets();
  return 1;  // Assuming always successful.
}

void EthernetClass::hardreset() {
  VLOG(3) << "EthernetClass::hardreset entry";
  CHECK_GE(reset_pin_, 0);
  CloseAllSockets();
}

void EthernetClass::setRstPin(uint8_t pinRST) {
  if (reset_pin_ < 0) {
    reset_pin_ = pinRST;
  } else {
    CHECK_EQ(reset_pin_, pinRST);
  }
}

void EthernetClass::setCsPin(uint8_t pinCS) {
  if (chip_select_pin_ < 0) {
    chip_select_pin_ = pinCS;
  } else {
    CHECK_EQ(chip_select_pin_, pinCS);
  }
}
