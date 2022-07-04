load(
    "//mcucore/extras/dev_tools:build_config.bzl",
    "build_config",
)

package(
    default_copts = build_config.default_copts,
)

cc_library(
    name = "addresses",
    srcs = ["addresses.cc"],
    hdrs = ["addresses.h"],
    deps = [
        ":eeprom_tags",
        ":ethernet_address",
        ":ip_address",
        "//mcucore/src:eeprom_region",
        "//mcucore/src:eeprom_tlv",
        "//mcucore/src:logging",
        "//mcucore/src:o_print_stream",
        "//mcucore/src:progmem_string_data",
        "//mcucore/src:progmem_string_view",
        "//mcucore/src:status",
        "//mcucore/src:status_or",
    ],
)

cc_library(
    name = "connection",
    srcs = ["connection.cc"],
    hdrs = ["connection.h"],
    deps = [
        ":platform_network",
        "//mcucore/src:limits",
        "//mcucore/src:logging",
    ],
)

cc_library(
    name = "disconnect_data",
    srcs = ["disconnect_data.cc"],
    hdrs = ["disconnect_data.h"],
    deps = [
        "//mcucore/src:logging",
        "//mcucore/src:mcucore_platform",
    ],
)

cc_library(
    name = "eeprom_tags",
    srcs = ["eeprom_tags.cc"],
    hdrs = ["eeprom_tags.h"],
    deps = ["//mcucore/src:eeprom_tag"],
)

cc_library(
    name = "ethernet_address",
    srcs = ["ethernet_address.cc"],
    hdrs = ["ethernet_address.h"],
    deps = [
        "//mcucore/src:eeprom_region",
        "//mcucore/src:hex_escape",
        "//mcucore/src:logging",
        "//mcucore/src:mcucore_platform",
        "//mcucore/src:progmem_string_data",
        "//mcucore/src:progmem_string_view",
        "//mcucore/src:status",
    ],
)

cc_library(
    name = "ip_address",
    srcs = ["ip_address.cc"],
    hdrs = ["ip_address.h"],
    deps = [
        ":platform_network",
        "//mcucore/src:eeprom_region",
        "//mcucore/src:logging",
        "//mcucore/src:mcucore_platform",
        "//mcucore/src:progmem_string_view",
        "//mcucore/src:status",
    ],
)

cc_library(
    name = "ip_device",
    srcs = ["ip_device.cc"],
    hdrs = ["ip_device.h"],
    deps = [
        ":addresses",
        ":ethernet_address",
        ":ip_address",
        ":platform_network",
        "//mcucore/src:eeprom_tlv",
        "//mcucore/src:log_sink",
        "//mcucore/src:logging",
        "//mcucore/src:mcucore_platform",
        "//mcucore/src:progmem_string_data",
        "//mcucore/src:progmem_string_view",
        "//mcucore/src:status",
    ],
)

cc_library(
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
    ],
)

cc_library(
    name = "mcunet_config",
    hdrs = ["mcunet_config.h"],
    deps = ["//mcucore/src:mcucore_platform"],
)

cc_library(
    name = "platform_network",
    srcs = ["platform_network.cc"],
    hdrs = ["platform_network.h"],
    deps = [
        ":mcunet_config",
        ":platform_network_interface",
        "//mcucore/src:logging",
        "//mcucore/src:mcucore_platform",
        "//mcunet/extras/host/arduino:client",
        "//mcunet/extras/host/arduino:ip_address",
        "//mcunet/extras/host/ethernet5500",
        "//mcunet/extras/host/ethernet5500:host_sockets",
    ],
)

cc_library(
    name = "platform_network_interface",
    srcs = ["platform_network_interface.cc"],
    hdrs = ["platform_network_interface.h"],
    textual_hdrs = ["platform_network_api.cc.inc"],
    deps = [
        ":mcunet_config",
        "//mcucore/src:logging",
        "//mcucore/src:mcucore_platform",
    ],
)

cc_library(
    name = "server_socket",
    srcs = ["server_socket.cc"],
    hdrs = ["server_socket.h"],
    deps = [
        ":disconnect_data",
        ":platform_network",
        ":socket_listener",
        ":tcp_server_connection",
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

cc_library(
    name = "tcp_server_connection",
    srcs = ["tcp_server_connection.cc"],
    hdrs = ["tcp_server_connection.h"],
    deps = [
        ":connection",
        ":disconnect_data",
        ":platform_network",
        "//mcucore/src:logging",
        "//mcucore/src:mcucore_platform",
    ],
)
