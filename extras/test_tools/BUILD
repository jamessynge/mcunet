# Libraries intended to simplify the task of writing tests of McuNet.

package(
    # In addition to testonly, I'd also like to have a "hostonly" feature on a
    # target, so that I could readily mark those features that can be used on
    # embedded systems, and those that are only available on host systems. There
    # probably is some notion of "platform" that I can use. TBD.
    default_testonly = 1,
)

cc_library(
    name = "fake_platform_network",
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
    hdrs = ["mock_client.h"],
    deps = [
        "//googletest:gunit_headers",
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
    name = "mock_ethernet_udp",
    hdrs = ["mock_ethernet_udp.h"],
    deps = [
        "//googletest:gunit_headers",
        "//mcunet/extras/host/arduino:ip_address",
        "//mcunet/extras/host/ethernet5500:ethernet_udp",
    ],
)

cc_library(
    name = "mock_platform_network",
    hdrs = ["mock_platform_network.h"],
    deps = [
        "//googletest:gunit_headers",
        "//mcunet/src:platform_network",
        "//mcunet/src:platform_network_interface",
    ],
)

cc_library(
    name = "mock_request_decoder_listener",
    srcs = ["mock_request_decoder_listener.cc"],
    hdrs = ["mock_request_decoder_listener.h"],
    deps = [
        "//googletest:gunit_headers",
        "//mcucore/extras/test_tools:progmem_string_utils",
        "//mcucore/extras/test_tools:string_view_utils",
        "//mcunet/src/http1:request_decoder",
        "//mcunet/src/http1:request_decoder_constants",
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

cc_library(
    name = "string_utils",
    srcs = ["string_utils.cc"],
    hdrs = ["string_utils.h"],
    deps = [
        "//absl/log",
        "//absl/log:check",
        "//absl/strings",
        "//mcucore/src:string_view",
    ],
)
