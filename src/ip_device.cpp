// TODO(jamessynge): Ethernet.init should take the chip select pin as an arg,
// and should call W5500::init so that we can then do a softReset call.
// TODO(jamessynge): EthernetClass should use millis() and a static var
// to decide whether it has been long enough since power-up or hard-reset so
// that we can rely on the chip being ready to work.
// TODO(jamessynge): Add some means of testing whether there is an Ethernet
// cable attached. In the not, or in the event that DHCP doesn't work, we may
// want to keep trying to initialize the networking from the main loop, at least
// until we see some traffic.

#include "ip_device.h"

#include <McuCore.h>

#include "platform_ethernet.h"

namespace mcunet {
namespace {

constexpr uint8_t kW5500ChipSelectPin = 10;
constexpr uint8_t kW5500ResetPin = 7;
constexpr uint8_t kSDcardSelectPin = 4;

static ::DhcpClass dhcp;

}  // namespace

// static
void Mega2560Eth::SetW5500Configuration(uint8_t max_sock_num) {
  // Configure Ethernet5500's EthernetClass instance with the pins used to
  // access the WIZnet W5500.
  Ethernet.setRstPin(kW5500ResetPin);
  Ethernet.setCsPin(kW5500ChipSelectPin);

  // Configure Ethernet5500's EthernetClass instance with the number of hardware
  // sockets to allow.
  Ethernet.init(max_sock_num);
}

// static
bool Mega2560Eth::ResetW5500() {
  MCU_DCHECK_EQ(Ethernet._pinRST, kW5500ResetPin);
  MCU_DCHECK_EQ(Ethernet._pinCS, kW5500ChipSelectPin);

  // Make sure that the SD Card interface is not the selected SPI device.
  pinMode(kSDcardSelectPin, OUTPUT);
  digitalWrite(kSDcardSelectPin, HIGH);

  // TODO(jamessynge): Check for presence of W5500 chip by reading the
  // VERSIONR register, which should return a value of 4 according to the
  // datasheet.

  // If there has been a crash and restart of the ATmega, I've found that the
  // networking seems to be broken, so doing a hard reset explicitly so that
  // we always act more like a power-up situation.
  // TODO(jamessynge): Determine if the RobotDyn Mega ETH has the reset pin
  // connected by default, or if a solder bridge needs to be created.
  Ethernet.hardreset();

  // It appears that hardreset doesn't always work... in particular, I find that
  // after uploading a sketch several times UDP discovery packets aren't
  // received anymore, and the networking chip *may* be only partially working.
  // So, attempt to force at least a soft reset of the device, i.e. one where it
  // is supposed to reset all of its internal registers to zero.
  return Ethernet.softreset() != 0;
}

IpDevice::EStatus IpDevice::StartNetworking(const uint8_t max_sock_num,
                                            const OuiPrefix* oui_prefix) {
  status_ = kNoHardware;
  Mega2560Eth::SetW5500Configuration(max_sock_num);
  if (Mega2560Eth::ResetW5500()) {
    status_ = InitializeNetworking(oui_prefix);
  }
  return status_;
}

IpDevice::EStatus IpDevice::InitializeNetworking(const OuiPrefix* oui_prefix) {
  // Load the addresses saved to EEPROM, if they were previously saved. If
  // they were not successfully loaded, then generate them and save them into
  // the EEPROM.
  Addresses addresses;
  addresses.loadOrGenAndSave(oui_prefix);

  // If unable to get an address using DHCP, see if we have a cable attached.
  // try again with a softReset between the two attempts.
  Ethernet.setDhcp(&dhcp);
  if (Ethernet.begin(addresses.mac.mac)) {
    // Wonderful news, we were able to get an IP address via DHCP.
    return kLinkUpDhcpSuccessful;
  }

  // Is there hardware? If there is, we should be able to read our MAC address
  // back from the chip.
  MacAddress mac;
  Ethernet.macAddress(mac.mac);
  if (!(mac == addresses.mac)) {
    // Oops, this isn't the right board to run this sketch.
    mcucore::LogSink() << MCU_FLASHSTR("IP device missing");
    return kNoHardware;
  }

  if (Ethernet.link() == 0) {
    mcucore::LogSink() << MCU_FLASHSTR("No Ethernet link!");
    return kNoLink;
  }

  // No DHCP server responded with a lease on an IP address, so we'll fallback
  // to using our randomly generated IP.
  mcucore::LogSink() << MCU_FLASHSTR("No DHCP, using address ") << addresses.ip;

  // The link-local address range must not be divided into smaller subnets, so
  // we set our subnet mask accordingly:
  IPAddress subnet(255, 255, 0, 0);

  // We assume that the gateway is on the same subnet, at address 1 within the
  // subnet. This code will work with many subnets, not just a /16.
  IPAddress gateway = addresses.ip;
  gateway[0] &= subnet[0];
  gateway[1] &= subnet[1];
  gateway[2] &= subnet[2];
  gateway[3] &= subnet[3];
  gateway[3] |= 1;

  Ethernet.begin(addresses.mac.mac, addresses.ip, subnet, gateway);

  return kLinkUpDhcpFailed;
}

bool IpDevice::MaintainDhcpLease() {
  // If we're using an IP address assigned via DHCP, renew the lease
  // periodically. The Ethernet library will do so at the appropriate interval
  // if we call it often enough.
  if (status_ == kLinkUpDhcpSuccessful) {
    auto dhcp_status = Ethernet.maintain();
    if (dhcp_status == DHCP_CHECK_NONE || dhcp_status == DHCP_CHECK_RENEW_OK ||
        dhcp_status == DHCP_CHECK_REBIND_OK) {
      return true;
    }
    if (dhcp_status == DHCP_CHECK_RENEW_FAIL) {
      mcucore::LogSink() << MCU_FLASHSTR("DHCP RENEW failed");
    } else if (dhcp_status == DHCP_CHECK_REBIND_FAIL) {
      mcucore::LogSink() << MCU_FLASHSTR("DHCP REBIND failed");
    } else {
      mcucore::LogSink() << MCU_FLASHSTR("DHCP failed, status = ")
                         << dhcp_status;
    }
  } else if (IsLinked()) {
    // Make sure the link is still working.
    if (Ethernet.link() != 0) {
      return true;
    }
    mcucore::LogSink() << MCU_FLASHSTR("Ethernet link lost!");
  } else {
    return false;
  }
  status_ = kNoHardware;
  return false;
}

void IpDevice::PrintNetworkAddresses() {
  MacAddress mac;
  Ethernet.macAddress(mac.mac);
  mcucore::LogSink() << MCU_FLASHSTR("MAC: ") << mac << '\n'
                     << MCU_FLASHSTR("IP: ") << Ethernet.localIP() << '\n'
                     << MCU_FLASHSTR("Subnet: ") << Ethernet.subnetMask()
                     << '\n'
                     << MCU_FLASHSTR("Gateway: ") << Ethernet.gatewayIP()
                     << '\n'
                     << MCU_FLASHSTR("DNS: ") << Ethernet.dnsServerIP();
}

}  // namespace mcunet
