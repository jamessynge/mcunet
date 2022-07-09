# Libraries intended to simplify the task of writing tests of McuNet.

cc_library(
    name = "fake_platform_network",
    testonly = True,
    hdrs = ["fake_platform_network.h"],
    deps = ["//mcunet/src:platform_network_interface"],
)

cc_library(
    name = "fake_write_buffered_connection",
    hdrs = ["fake_write_buffered_connection.h"],
    deps = [
        "//googletest:gunit_headers",
        "//mcunet/extras/host/arduino:client",
        "//mcunet/src:write_buffered_connection",
    ],
)

cc_library(
    name = "mock_client",
    testonly = True,
    hdrs = ["mock_client.h"],
    deps = [
        "//googletest:gunit_headers",
        "//mcunet/extras/host/arduino:client",
        "//mcunet/extras/host/arduino:ip_address",
    ],
)

cc_library(
    name = "mock_ethernet_client",
    testonly = True,
    hdrs = ["mock_ethernet_client.h"],
    deps = [
        "//googletest:gunit_headers",
        "//mcunet/extras/host/arduino:ip_address",
        "//mcunet/extras/host/ethernet5500:ethernet_client",
    ],
)

cc_library(
    name = "mock_ethernet_udp",
    testonly = True,
    hdrs = ["mock_ethernet_udp.h"],
    deps = [
        "//googletest:gunit_headers",
        "//mcunet/extras/host/arduino:ip_address",
        "//mcunet/extras/host/ethernet5500:ethernet_udp",
    ],
)

cc_library(
    name = "mock_platform_network",
    testonly = True,
    hdrs = ["mock_platform_network.h"],
    deps = [
        "//googletest:gunit_headers",
        "//mcunet/src:platform_network",
        "//mcunet/src:platform_network_interface",
    ],
)

cc_library(
    name = "mock_socket_listener",
    testonly = True,
    hdrs = ["mock_socket_listener.h"],
    deps = [
        "//googletest:gunit_headers",
        "//mcunet/src:connection",
        "//mcunet/src:socket_listener",
    ],
)

cc_library(
    name = "string_io_stream_impl",
    hdrs = ["string_io_stream_impl.h"],
    deps = [
        "//mcucore/extras/host/arduino:stream",
        "//mcunet/extras/host/arduino:client",
        "//mcunet/src:connection",
    ],
)

cc_test(
    name = "string_io_stream_impl_test",
    srcs = ["string_io_stream_impl_test.cc"],
    deps = [
        ":string_io_stream_impl",
        "//googletest:gunit_main",
    ],
)
