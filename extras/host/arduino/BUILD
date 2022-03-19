# This package contains a subset of the Arduino libraries related to networking,
# for use when compiling on for a host target (e.g. Linux) rather than for an
# embedded target.

cc_library(
    name = "client",
    hdrs = ["client.h"],
    deps = [
        ":ip_address",
        "//mcucore/extras/host/arduino:stream",
    ],
)

cc_library(
    name = "ip_address",
    hdrs = ["ip_address.h"],
    deps = [
        "//mcucore/extras/host/arduino:print",
    ],
)
