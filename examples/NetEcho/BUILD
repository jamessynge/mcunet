# Sketch for echoing what it receives over a TCP socket.

load(
    "//mcucore/extras/bazel:arduino_cc_sketch.bzl",
    "arduino_cc_sketch",
)

arduino_cc_sketch(
    name = "NetEcho",
    srcs = ["NetEcho.ino.cc"],
    deps = [
        "//mcucore/src:McuCore",
        "//mcunet/extras/host/ethernet5500:host_network_main",
        "//mcunet/src:mcu_net",
        "//mcunet/src:server_socket",
    ],
)
