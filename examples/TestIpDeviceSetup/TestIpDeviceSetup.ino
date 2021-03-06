#include <Arduino.h>
#include <Ethernet5500.h>
#include <McuCore.h>
#include <McuNet.h>

mcunet::IpDevice ip_device;

void announceFailure(const char* message) {
  while (true) {
    Serial.println(message);
    delay(1000);
  }
}

void announceAddresses() {
  Serial.println();
  mcunet::IpDevice::PrintNetworkAddresses();
  Serial.println();
}

void setup() {
  // Setup serial, wait for it to be ready so that our logging messages can be
  // read.
  Serial.begin(115200);
  // Wait for serial port to connect, or at least some minimum amount of time
  // (TBD), else the initial output gets lost.
  while (!Serial) {
  }

  //////////////////////////////////////////////////////////////////////////////
  // Initialize networking.
  mcunet::Mega2560Eth::SetupW5500();

  // Get an EepromTlv instance, to be used for persistence of settings.
  auto eeprom_tlv = mcucore::EepromTlv::GetOrDie();

  // Initialize the pseudo-random number generator with a random number
  // generated based on clock jitter.
  mcucore::JitterRandom::setRandomSeed();

  // Provide an "Organizationally Unique Identifier" which will be used as the
  // first 3 bytes of the MAC addresses generated; this means that all boards
  // running this sketch will share the first 3 bytes of their MAC addresses,
  // which may help with locating them.
  mcunet::OuiPrefix oui_prefix(0x53, 0x75, 0x76);
  MCU_CHECK_OK(ip_device.InitializeNetworking(eeprom_tlv, &oui_prefix));

  announceAddresses();

  for (int sock_num = 0; sock_num < Ethernet._maxSockNum; ++sock_num) {
    Serial.print("Socket ");
    Serial.print(sock_num);
    Serial.print(" status: ");
    EthernetClient client(sock_num);
    auto status = client.status();
    switch (status) {
      case SnSR::CLOSED:
        Serial.println("CLOSED");
        break;
      case SnSR::INIT:
        Serial.println("INIT");
        break;
      case SnSR::LISTEN:
        Serial.println("LISTEN");
        break;
      case SnSR::SYNSENT:
        Serial.println("SYNSENT");
        break;
      case SnSR::SYNRECV:
        Serial.println("SYNRECV");
        break;
      case SnSR::ESTABLISHED:
        Serial.println("ESTABLISHED");
        break;
      case SnSR::FIN_WAIT:
        Serial.println("FIN_WAIT");
        break;
      case SnSR::CLOSING:
        Serial.println("CLOSING");
        break;
      case SnSR::TIME_WAIT:
        Serial.println("TIME_WAIT");
        break;
      case SnSR::CLOSE_WAIT:
        Serial.println("CLOSE_WAIT");
        break;
      case SnSR::LAST_ACK:
        Serial.println("LAST_ACK");
        break;
      case SnSR::UDP:
        Serial.println("UDP");
        break;
      case SnSR::IPRAW:
        Serial.println("IPRAW");
        break;
      case SnSR::MACRAW:
        Serial.println("MACRAW");
        break;
      case SnSR::PPPOE:
        Serial.println("PPPOE");
        break;
      default:
        Serial.println(status);
    }
  }
}

void loop() {
  auto dhcp_check = ip_device.MaintainDhcpLease();
  switch (dhcp_check) {
    case DHCP_CHECK_NONE:
      break;
    case DHCP_CHECK_RENEW_FAIL:
      Serial.println("DHCP_CHECK_RENEW_FAIL: Unable to renew the DHCP lease.");
      delay(1000);
      return;
    case DHCP_CHECK_RENEW_OK:
      Serial.println("DHCP_CHECK_RENEW_OK");
      return;
    case DHCP_CHECK_REBIND_FAIL:
      Serial.println("DHCP_CHECK_REBIND_FAIL: Unable to renew the DHCP lease.");
      delay(1000);
      return;
    case DHCP_CHECK_REBIND_OK:
      Serial.println("DHCP_CHECK_REBIND_OK");
      announceAddresses();
      return;
    default:
      Serial.print("Unexpected result from MaintainDhcpLease: ");
      Serial.println(dhcp_check);
  }
}
