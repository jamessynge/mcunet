# Sketch for testing that we can initialize the IpDevice (i.e. WIZnet W5500).

cc_binary(
    name = "TestIpDeviceSetup",
    srcs = ["TestIpDeviceSetup.ino.cc"],
    deps = [
        "//mcucore/extras/host/arduino",
        "//mcucore/src:McuCore",
        "//mcucore/src/eeprom:eeprom_tlv",
        "//mcunet/extras/host/ethernet5500",
        "//mcunet/extras/host/ethernet5500:host_network_main",
        "//mcunet/src:mcu_net",
    ],
)
