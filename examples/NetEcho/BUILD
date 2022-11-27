# Sketch for echoing what it receives over a TCP socket.

load(
    "//experimental/users/jamessynge/arduino/tas/extras/dev_tools:build_config.bzl",
    "build_config",
)

package(
    default_copts = build_config.default_copts,
)

cc_binary(
    name = "NetEcho",
    srcs = ["NetEcho.ino.cc"],
    deps = [
        "//mcucore/src:McuCore",
        "//mcunet/extras/host/ethernet5500:host_network_main",
        "//mcunet/src:mcu_net",
        "//mcunet/src:server_socket",
    ],
)
