# A fake host version of the Ethernet5500 Arduino Library, itself forked
# from sstaub's Ethernet3 for the WIZ5500. Has just enough for compiling
# and testing Tiny Alpaca Server on Linux host (i.e. a host with the usual
# socket APIs).

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
        "//mcunet/extras/host/arduino:ip_address",
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
        "//mcucore/src:logging",
        "//mcunet/extras/host/arduino:ip_address",
        "//mcunet/src:platform_network_interface",
    ],
)

cc_library(
    name = "ethernet_client",
    srcs = ["ethernet_client.cc"],
    hdrs = ["ethernet_client.h"],
    deps = [
        "//absl/log",
        "//absl/log:check",
        "//mcucore/src:logging",
        "//mcunet/extras/host/arduino:client",
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
        ":ethernet_client",
        ":ethernet_config",
        "//mcucore/extras/host/arduino:stream",
        "//mcunet/extras/host/arduino:ip_address",
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
        "//mcucore/src:logging",
        "//mcunet/src:platform_network",
        "//mcunet/src:platform_network_interface",
    ],
)

cc_library(
    name = "host_socket_info",
    srcs = ["host_socket_info.cc"],
    hdrs = ["host_socket_info.h"],
    deps = ["//glog:logging"],
)

cc_library(
    name = "w5500",
    hdrs = ["w5500.h"],
)
