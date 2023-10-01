load(
    "//mcucore/extras/bazel:arduino_cc_library.bzl",
    "arduino_cc_library",
)

arduino_cc_library(
    name = "addresses",
    srcs = ["addresses.cc"],
    hdrs = ["addresses.h"],
    deps = [
        ":eeprom_tags",
        ":ethernet_address",
        ":ip_address",
        "//mcucore/src/eeprom:eeprom_region",
        "//mcucore/src/eeprom:eeprom_tlv",
        "//mcucore/src/log",
        "//mcucore/src/print:o_print_stream",
        "//mcucore/src/status",
        "//mcucore/src/status:status_or",
        "//mcucore/src/strings:progmem_string_view",
    ],
)

arduino_cc_library(
    name = "connection",
    srcs = ["connection.cc"],
    hdrs = ["connection.h"],
    deps = [
        ":platform_network",
        "//mcucore/extras/host/arduino:stream",
    ],
)

arduino_cc_library(
    name = "disconnect_data",
    srcs = ["disconnect_data.cc"],
    hdrs = ["disconnect_data.h"],
    deps = [
        "//mcucore/src:mcucore_platform",
        "//mcucore/src/log",
    ],
)

arduino_cc_library(
    name = "eeprom_tags",
    srcs = ["eeprom_tags.cc"],
    hdrs = ["eeprom_tags.h"],
    deps = ["//mcucore/src/eeprom:eeprom_tag"],
)

arduino_cc_library(
    name = "ethernet_address",
    srcs = ["ethernet_address.cc"],
    hdrs = ["ethernet_address.h"],
    deps = [
        "//mcucore/src:mcucore_platform",
        "//mcucore/src/eeprom:eeprom_region",
        "//mcucore/src/log",
        "//mcucore/src/print:hex_escape",
        "//mcucore/src/status",
        "//mcucore/src/strings:progmem_string_data",
        "//mcucore/src/strings:progmem_string_view",
    ],
)

arduino_cc_library(
    name = "ip_address",
    srcs = ["ip_address.cc"],
    hdrs = ["ip_address.h"],
    deps = [
        ":platform_network",
        "//mcucore/src:mcucore_platform",
        "//mcucore/src/eeprom:eeprom_region",
        "//mcucore/src/log",
        "//mcucore/src/status",
        "//mcucore/src/strings:progmem_string_view",
    ],
)

arduino_cc_library(
    name = "ip_device",
    srcs = ["ip_device.cc"],
    hdrs = ["ip_device.h"],
    deps = [
        ":addresses",
        ":ethernet_address",
        ":ip_address",
        ":platform_network",
        "//mcucore/src:mcucore_platform",
        "//mcucore/src/eeprom:eeprom_tlv",
        "//mcucore/src/log",
        "//mcucore/src/log:log_sink",
        "//mcucore/src/status",
        "//mcucore/src/strings:progmem_string_data",
        "//mcucore/src/strings:progmem_string_view",
        "//mcunet/extras/host/ethernet5500:ethernet_class",
    ],
)

arduino_cc_library(
    name = "mcu_net",
    hdrs = ["McuNet.h"],
    deps = [
        ":addresses",
        ":connection",
        ":disconnect_data",
        ":eeprom_tags",
        ":ethernet_address",
        ":ip_address",
        ":ip_device",
        ":mcunet_config",
        ":platform_network",
        ":platform_network_interface",
        ":server_socket",
        ":socket_listener",
        ":tcp_server_connection",
        ":write_buffered_connection",
    ],
)

arduino_cc_library(
    name = "mcunet_config",
    hdrs = ["mcunet_config.h"],
    deps = ["//mcucore/src:mcucore_platform"],
)

arduino_cc_library(
    name = "platform_network",
    srcs = ["platform_network.cc"],
    hdrs = ["platform_network.h"],
    deps = [
        ":platform_network_interface",
        "//mcucore/src:mcucore_platform",
        "//mcucore/src/log",
        "//mcunet/extras/host/arduino:client",
        "//mcunet/extras/host/arduino:ip_address",
        "//mcunet/extras/host/ethernet5500",
    ],
)

arduino_cc_library(
    name = "platform_network_interface",
    srcs = ["platform_network_interface.cc"],
    hdrs = ["platform_network_interface.h"],
    textual_hdrs = ["platform_network_api.cc.inc"],
    deps = [
        ":mcunet_config",
        "//mcucore/src/log",
    ],
)

arduino_cc_library(
    name = "server_socket",
    srcs = ["server_socket.cc"],
    hdrs = ["server_socket.h"],
    deps = [
        ":disconnect_data",
        ":platform_network",
        ":socket_listener",
        ":tcp_server_connection",
        "//mcucore/src:mcucore_platform",
        "//mcucore/src/log",
        "//mcucore/src/strings:progmem_string_data",
    ],
)

arduino_cc_library(
    name = "socket_listener",
    hdrs = ["socket_listener.h"],
    deps = [
        ":connection",
        "//mcucore/src:mcucore_platform",
    ],
)

arduino_cc_library(
    name = "tcp_server_connection",
    srcs = ["tcp_server_connection.cc"],
    hdrs = ["tcp_server_connection.h"],
    deps = [
        ":disconnect_data",
        ":platform_network",
        ":write_buffered_connection",
        "//mcucore/src:mcucore_platform",
        "//mcucore/src/log",
    ],
)

arduino_cc_library(
    name = "write_buffered_connection",
    srcs = ["write_buffered_connection.cc"],
    hdrs = ["write_buffered_connection.h"],
    deps = [
        ":connection",
        "//mcucore/src/log",
        "//mcucore/src/strings:progmem_string_data",
        "//mcunet/extras/host/arduino:client",
    ],
)
