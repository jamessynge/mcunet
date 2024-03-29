// TODO(jamessynge): Ethernet.init should take the chip select pin as an arg,
// and should call W5500::init so that we can then do a softReset call.
//
// TODO(jamessynge): EthernetClass should use millis() and a static var to
// decide whether it has been long enough since power-up or hard-reset so that
// we can rely on the chip being ready to work.
//
// TODO(jamessynge): Add some means of testing whether there is an Ethernet
// cable attached. In the not, or in the event that DHCP doesn't work, we may
// want to keep trying to initialize the networking from the main loop, at least
// until we see some traffic.

#include "ip_device.h"

#include <McuCore.h>

#include "addresses.h"
#include "ethernet_address.h"
#include "ip_address.h"
#include "platform_network.h"

namespace mcunet {
namespace {

constexpr uint8_t kW5500ChipSelectPin = 10;
constexpr uint8_t kW5500ResetPin = 7;
constexpr uint8_t kSDcardSelectPin = 4;

static ::DhcpClass dhcp;

}  // namespace

// static
void Mega2560Eth::SetupW5500(uint8_t max_sock_num) {
  MCU_VLOG(3) << MCU_PSD("SetupW5500 Entry");

  // Make sure that the SD Card interface is not the selected SPI device.
  pinMode(kSDcardSelectPin, OUTPUT);
  digitalWrite(kSDcardSelectPin, HIGH);

  // Configure Ethernet5500's EthernetClass instance with the pins used to
  // access the WIZnet W5500.
  Ethernet.setRstPin(kW5500ResetPin);
  Ethernet.setCsPin(kW5500ChipSelectPin);

  // If there has been a crash and restart of the ATmega, I've found that the
  // networking seems to be broken, so doing a hard reset explicitly so that we
  // always act more like a power-up situation.
  MCU_VLOG(3) << MCU_PSD("hardreset");
  Ethernet.hardreset();

  // For now use all of the allowed sockets. Need to have at least one UDP
  // socket, and maybe more; our UDP uses include DHCP lease & lease renewal,
  // the Alpaca discovery protocol, and possibly for time. Then we need at least
  // one TCP socket, more if we want to handle multiple simultaneous requests.
  MCU_VLOG(3) << MCU_PSD("init");
  Ethernet.init(max_sock_num);

  // Used to call Ethernet.softreset() here, in an attempt to deal with the hard
  // reset above not working (e.g. when the jumper/solder bridge is not present
  // to connect the reset W5500 reset pin to the kW5500ResetPin, an AVR GPIO
  // pin). However I found that this would sometimes hang, so not worth the
  // risk.
  MCU_VLOG(3) << MCU_PSD("SetupW5500 Exit");
}

mcucore::Status IpDevice::InitializeNetworking(
    mcucore::EepromTlv& eeprom_tlv, const OuiPrefix* const oui_prefix) {
  // Load the addresses saved to EEPROM, if they were previously saved. If they
  // were not successfully loaded, then generate them and save them into the
  // EEPROM.
  Addresses addresses;
  auto status = addresses.ReadEepromEntry(eeprom_tlv, oui_prefix);
  if (!status.ok()) {
    MCU_VLOG(2) << MCU_PSD("Error loading network addresses: ") << status;
    MCU_VLOG(1) << MCU_PSD("Generating Ethernet and default IP addresses");

    // Need to generate a new address.
    addresses.GenerateAddresses(oui_prefix);

    status = addresses.WriteEepromEntry(eeprom_tlv);
    MCU_DCHECK_OK(status) << MCU_PSD(
        "Failed to save generated network addresses");
    if (!status.ok()) {
      MCU_VLOG(1) << MCU_PSD("Failed to save generated network addresses")
                  << MCU_PSD(": ") << status;
    }
  }

  // If unable to get an address using DHCP, try again with a softReset
  // between the two attempts.
  Ethernet.setDhcp(&dhcp);
  using_dhcp_ = Ethernet.begin(addresses.ethernet.bytes);
  if (!using_dhcp_ && false) {
    MCU_VLOG(1) << MCU_PSD("Failed to get an address using DHCP");
    // TODO(jamessynge): First check whether there is an Ethernet cable
    // attached; if not, then we don't benefit from a retry. Instead, we can
    // check whether a cable is attached later in the main loop. This may
    // require splitting TinyAlpacaServer::initialize into two parts: one for
    // the networking hardware, repeated as necessary, and another (once only)
    // for the Alpaca devices (but not the Alpaca network servers). OR just loop
    // here indefinitely, waiting for the network cable to be attached.
    Ethernet.softreset();
    MCU_VLOG(1) << MCU_PSD("softreset complete");
    using_dhcp_ = Ethernet.begin(addresses.ethernet.bytes);
    if (!using_dhcp_) {
      MCU_VLOG(1) << MCU_PSD("Failed to get an address using DHCP")
                  << MCU_PSD(" after a soft reset.");
    }
  }

  if (!using_dhcp_) {
    // Is there hardware? If there is, we should be able to read our MAC address
    // back from the chip.
    EthernetAddress mac;
    Ethernet.macAddress(mac.bytes);
    if (!(mac == addresses.ethernet)) {
      // Oops, this isn't the right board to run this sketch.
      return mcucore::NotFoundError(MCU_PSV("Found no networking hardware"));
    }

    mcucore::LogSink() << MCU_PSD("No DHCP, using default IP ") << addresses.ip;

    // No DHCP server responded with a lease on an IP address, so we'll fallback
    // to using our randomly generated IP.
    using_dhcp_ = false;

    // The link-local address range must not be divided into smaller subnets, so
    // we set our subnet mask accordingly:
    IPAddress subnet(255, 255, 0, 0);

    // Assume that the gateway is on the same subnet, at address 1 within the
    // subnet. This code will work with many subnets, not just a /16.
    IPAddress gateway = addresses.ip;
    gateway[0] &= subnet[0];
    gateway[1] &= subnet[1];
    gateway[2] &= subnet[2];
    gateway[3] &= subnet[3];
    gateway[3] |= 1;

    Ethernet.begin(addresses.ethernet.bytes, addresses.ip, subnet, gateway);
  }

  return mcucore::OkStatus();
}

int IpDevice::MaintainDhcpLease() {
  // If we're using an IP address assigned via DHCP, renew the lease
  // periodically. The Ethernet library will do so at the appropriate interval
  // if we call it often enough.
  if (using_dhcp_) {
    return Ethernet.maintain();
  } else {
    return DHCP_CHECK_NONE;
  }
}

void IpDevice::PrintNetworkAddresses() {
  EthernetAddress mac;
  Ethernet.macAddress(mac.bytes);
  mcucore::LogSink() << MCU_PSD("MAC: ") << mac;
  mcucore::LogSink() << MCU_PSD("IP: ") << Ethernet.localIP();
  mcucore::LogSink() << MCU_PSD("Subnet: ") << Ethernet.subnetMask();
  mcucore::LogSink() << MCU_PSD("Gateway: ") << Ethernet.gatewayIP();
  mcucore::LogSink() << MCU_PSD("DNS: ") << Ethernet.dnsServerIP();
}

}  // namespace mcunet
