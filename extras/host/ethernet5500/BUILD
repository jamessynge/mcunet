# A minimal host version of the Ethernet5500 Arduino Library, itself forked
# from sstaub's Ethernet3 for the WIZ5500. For the basic TCP server features,
# uses PlatformNetwork to get the networking implementation to use, thus
# allowing both mocking and a "real" TCP implementation to be used for testing.
# For other features, such as DHCP, there is essentially no implementation,
# maybe just enough for compiling.

cc_library(
    name = "dhcp_class",
    hdrs = ["dhcp_class.h"],
)

cc_library(
    name = "ethernet5500",
    hdrs = ["ethernet5500.h"],
    deps = [
        ":dhcp_class",
        ":ethernet_class",
        ":ethernet_client",
        ":ethernet_config",
        ":ethernet_server",
        ":ethernet_udp",
        ":w5500",
        "//mcucore/extras/host/arduino:ip_address",
    ],
)

cc_library(
    name = "ethernet_class",
    srcs = ["ethernet_class.cc"],
    hdrs = ["ethernet_class.h"],
    deps = [
        ":dhcp_class",
        ":ethernet_config",
        "//absl/log",
        "//absl/log:check",
        "//mcucore/extras/host/arduino:ip_address",
        "//mcunet/src:platform_network_interface",
    ],
)

cc_library(
    name = "ethernet_client",
    srcs = ["ethernet_client.cc"],
    hdrs = ["ethernet_client.h"],
    deps = [
        ":w5500",
        "//absl/log",
        "//absl/log:check",
        "//mcucore/extras/host/arduino:client",
        "//mcucore/extras/host/arduino:ip_address",
        "//mcunet/src:platform_network_interface",
    ],
)

cc_library(
    name = "ethernet_config",
    hdrs = ["ethernet_config.h"],
)

cc_library(
    name = "ethernet_server",
    srcs = ["ethernet_server.cc"],
    hdrs = ["ethernet_server.h"],
    deps = [
        ":ethernet_class",
        ":ethernet_client",
        ":ethernet_config",
        ":w5500",
        "//absl/log",
        "//absl/log:check",
        "//mcunet/src:platform_network_interface",
    ],
)

cc_library(
    name = "ethernet_udp",
    srcs = ["ethernet_udp.cc"],
    hdrs = ["ethernet_udp.h"],
    deps = [
        "//mcucore/extras/host/arduino:ip_address",
        "//mcucore/extras/host/arduino:stream",
    ],
)

cc_library(
    name = "host_network",
    srcs = ["host_network.cc"],
    hdrs = ["host_network.h"],
    deps = [
        ":host_socket_info",
        "//absl/log",
        "//absl/log:check",
        "//absl/strings",
        "//mcunet/src:platform_network_interface",
    ],
)

cc_library(
    name = "host_network_main",
    srcs = ["host_network_main.cc"],
    linkopts = [
        # Don't complain that setup and loop aren't defined. They'll be provided
        # by the cc_binary that links in this library, by way of the setup and
        # loop functions in the source of the sketch (.ino file).
        "-Wl,--undefined=setup",
        "-Wl,--undefined=loop",
    ],
    deps = [
        ":ethernet_client",
        ":host_network",
        "//absl/log",
        "//base",
        "//mcucore/extras/host/arduino:call_setup_and_loop",
        "//mcunet/src:platform_network_interface",
    ],
)

cc_library(
    name = "host_socket_info",
    srcs = ["host_socket_info.cc"],
    hdrs = ["host_socket_info.h"],
    deps = [
        "//absl/flags:flag",
        "//absl/flags:marshalling",
        "//absl/log",
        "//absl/log:check",
        "//absl/strings",
        "//mcucore/extras/host:posix_errno",
    ],
)

cc_library(
    name = "w5500",
    hdrs = ["w5500.h"],
)
