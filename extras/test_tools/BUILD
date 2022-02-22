# Libraries intended to simplify the task of writing tests of McuNet.

cc_library(
    name = "fake_platform_ethernet",
    hdrs = ["fake_platform_ethernet.h"],
    deps = ["//mcunet/src:platform_ethernet"],
)

cc_library(
    name = "mock_client",
    hdrs = ["mock_client.h"],
    deps = [
        "//googletest:gunit_headers",
        "//mcucore/extras/test_tools:mock_stream",
        "//mcunet/extras/host/arduino:client",
        "//mcunet/extras/host/arduino:ip_address",
    ],
)

cc_library(
    name = "mock_ethernet_client",
    hdrs = ["mock_ethernet_client.h"],
    deps = [
        "//googletest:gunit_headers",
        "//mcunet/extras/host/arduino:ip_address",
        "//mcunet/extras/host/ethernet5500:ethernet_client",
    ],
)

cc_library(
    name = "mock_socket_listener",
    hdrs = ["mock_socket_listener.h"],
    deps = [
        "//googletest:gunit_headers",
        "//mcunet/src:connection",
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
