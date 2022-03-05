# Libraries intended to simplify the task of writing tests of McuNet.

cc_library(
    name = "fake_platform_ethernet",
    testonly = True,
    hdrs = ["fake_platform_ethernet.h"],
    deps = ["//mcunet/src:platform_ethernet"],
)

cc_library(
    name = "mock_client",
    testonly = True,
    hdrs = ["mock_client.h"],
    deps = [
        "//googletest:gunit_headers",
        "//mcunet/extras/host/arduino:client",
    ],
)

cc_library(
    name = "mock_ethernet_client",
    testonly = True,
    hdrs = ["mock_ethernet_client.h"],
    deps = [
        "//googletest:gunit_headers",
        "//mcunet/extras/host/ethernet5500:ethernet_client",
    ],
)

cc_library(
    name = "mock_ethernet_udp",
    testonly = True,
    hdrs = ["mock_ethernet_udp.h"],
    deps = [
        "//googletest:gunit_headers",
        "//mcunet/extras/host/ethernet5500:ethernet_udp",
    ],
)

cc_library(
    name = "mock_socket_listener",
    testonly = True,
    hdrs = ["mock_socket_listener.h"],
    deps = [
        "//googletest:gunit_headers",
        "//mcunet/src:socket_listener",
    ],
)

cc_library(
    name = "string_io_connection",
    hdrs = ["string_io_connection.h"],
    deps = ["//mcunet/src:connection"],
)

cc_test(
    name = "string_io_connection_test",
    srcs = ["string_io_connection_test.cc"],
    deps = [
        ":string_io_connection",
        "//googletest:gunit_main",
    ],
)
