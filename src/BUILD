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
        ":platform_ethernet",
        "//mcucore/src:crc32",
        "//mcucore/src:eeprom_io",
        "//mcucore/src:hex_escape",
        "//mcucore/src:jitter_random",
        "//mcucore/src:logging",
        "//mcucore/src:mcucore_platform",
        "//mcucore/src:o_print_stream",
        "//mcucore/src:progmem_string_data",
        "//mcucore/src:progmem_string_view",
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
        "//mcucore/src:log_sink",
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
        ":tcp_server_connection",
    ],
)

cc_library(
    name = "platform_ethernet",
    srcs = ["platform_ethernet.cc"],
    hdrs = ["platform_ethernet.h"],
    deps = [
        "//mcucore/src:logging",
        "//mcucore/src:mcucore_platform",
        "//mcunet/extras/host/arduino:client",
        "//mcunet/extras/host/arduino:ip_address",
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
        ":socket_listener",
        "//mcucore/src:logging",
        "//mcucore/src:mcucore_platform",
    ],
)
