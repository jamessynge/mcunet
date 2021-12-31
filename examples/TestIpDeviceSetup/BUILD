# Sketch for testing that we can initialize the IpDevice (i.e. WIZnet W5500).

cc_binary(
    name = "TestIpDeviceSetup",
    srcs = ["TestIpDeviceSetup.ino.cc"],
    deps = [
        "//mcucore/extras/host/arduino",
        "//mcucore/extras/host/arduino:arduino_main",
        "//mcucore/src:McuCore",
        "//mcunet/extras/host/ethernet5500",
        "//mcunet/src:mcu_net",
    ],
)
