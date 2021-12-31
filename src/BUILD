# TODO(jamessynge): describe this package.

cc_library(
    name = "addresses",
    srcs = ["addresses.cc"],
    hdrs = ["addresses.h"],
    deps = [
        ":platform_ethernet",
        "//mcucore/src:eeprom_io",
        "//mcucore/src:hex_escape",
        "//mcucore/src:inline_literal",
        "//mcucore/src:logging",
        "//mcucore/src:mcucore_platform",
        "//mcucore/src:o_print_stream",
        "//mcucore/src:progmem_string_data",
    ],
)

cc_library(
    name = "connection",
    srcs = ["connection.cc"],
    hdrs = ["connection.h"],
    deps = [
        ":platform_ethernet",
        "//mcucore/src:logging",
    ],
)

cc_library(
    name = "ip_device",
    srcs = ["ip_device.cc"],
    hdrs = ["ip_device.h"],
    deps = [
        ":addresses",
        ":platform_ethernet",
        "//mcucore/src:inline_literal",
        "//mcucore/src:logging",
        "//mcucore/src:mcucore_platform",
        "//mcucore/src:progmem_string_data",
    ],
)

cc_library(
    name = "mcu_net",
    hdrs = ["McuNet.h"],
    deps = [
        ":addresses",
        ":connection",
        ":ip_device",
        ":platform_ethernet",
        ":server_socket",
        ":socket_listener",
    ],
)

cc_library(
    name = "platform_ethernet",
    srcs = ["platform_ethernet.cc"],
    hdrs = ["platform_ethernet.h"],
    deps = [
        "//mcucore/extras/host/arduino:client",
        "//mcucore/extras/host/arduino:ip_address",
        "//mcucore/src:logging",
        "//mcucore/src:mcucore_platform",
        "//mcunet/extras/host/ethernet5500",
        "//mcunet/extras/host/ethernet5500:host_sockets",
    ],
)

cc_library(
    name = "server_socket",
    srcs = ["server_socket.cc"],
    hdrs = ["server_socket.h"],
    deps = [
        ":connection",
        ":platform_ethernet",
        ":socket_listener",
        "//mcucore/src:logging",
        "//mcucore/src:mcucore_platform",
        "//mcucore/src:o_print_stream",
    ],
)

cc_library(
    name = "socket_listener",
    hdrs = ["socket_listener.h"],
    deps = [
        ":connection",
        "//mcucore/src:mcucore_platform",
    ],
)
