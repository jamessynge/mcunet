#include <McuCore.h>
#include <McuNet.h>

// TODO(jamessynge): Describe why this sketch exists/what it demonstrates.

mcunet::IpDevice ip_device;

class EchoListener : public mcunet::ServerSocketListener {
 public:
  void OnConnect(mcunet::Connection& connection) override {
    MCU_VLOG(1) << MCU_PSD("OnConnect");
  }
  void OnCanRead(mcunet::Connection& connection) override {
    uint8_t buffer[128];
    int size = connection.read(buffer, sizeof(buffer));
    MCU_VLOG(1) << MCU_PSD("OnCanRead read -> ") << size;
    if (size > 0) {
      size = connection.write(buffer, size);
      MCU_VLOG(1) << MCU_PSD("OnCanRead write -> ") << size;
    }
  }
  void OnHalfClosed(mcunet::Connection& connection) override {
    MCU_VLOG(1) << MCU_PSD("OnHalfClosed, closing the connection");
    connection.close();
  }
  void OnDisconnect() override { MCU_VLOG(1) << MCU_PSD("OnDisconnect"); }
} echo_listener;  // NOLINT

mcunet::ServerSocket echo_socket(80, echo_listener);  // NOLINT

void setup() {
  // Setup serial, wait for it to be ready so that our logging messages can be
  // read. Note that the baud rate is meaningful on boards that do true serial,
  // while those microcontrollers with builtin USB likely don't rate limit
  // because there isn't a need.
  Serial.begin(115200);

  // Wait for serial port to connect, or at least some minimum amount of time
  // (TBD), else the initial output gets lost. Note that this isn't true for all
  // Arduino-like boards: some reset when the Serial Monitor connects, so we
  // almost always get the initial output. Note though that a software reset
  // such as that may not reset all of the hardware features, leading to hard
  // to diagnose bugs (experience speaking).
  while (!Serial) {
  }

  mcucore::LogSink() << MCU_PSD("NetEcho port 80");
  mcucore::LogSink() << MCU_PSD("Initializing networking");
  mcunet::Mega2560Eth::SetupW5500();

  // Initialize the pseudo-random number generator with a random number
  // generated based on clock jitter.
  mcucore::JitterRandom::setRandomSeed();

  // Provide an "Organizationally Unique Identifier" which will be used as the
  // first 3 bytes of the MAC addresses generated; this means that all boards
  // running this sketch will share the first 3 bytes of their MAC addresses,
  // which may help with locating them.
  mcunet::OuiPrefix oui_prefix(0x53, 0x76, 0x77);
  mcucore::EepromTlv eeprom_tlv = mcucore::EepromTlv::GetOrDie();
  MCU_CHECK_OK((ip_device.InitializeNetworking(eeprom_tlv, &oui_prefix)));

  Serial.println();
  mcunet::IpDevice::PrintNetworkAddresses();
  Serial.println();
}

void loop() {
  if (!echo_socket.HasSocket()) {
    if (echo_socket.PickClosedSocket()) {
      MCU_VLOG(1) << MCU_PSD("Picked a hardware socket for echo_socket.");
    } else {
      MCU_VLOG(1) << MCU_PSD("echo_socket::PickClosedSocket failed!");
    }
  }
  echo_socket.PerformIO();
}
