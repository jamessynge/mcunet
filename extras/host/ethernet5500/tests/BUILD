# Tests of the faux/limited host-only Ethernet5500 library.

cc_test(
    name = "ethernet_client_test",
    srcs = ["ethernet_client_test.cc"],
    deps = [
        "//googletest:gunit_main",
        "//mcucore/extras/test_tools:print_to_std_string",
        "//mcunet/extras/host/ethernet5500:ethernet_client",
    ],
)

cc_test(
    name = "host_network_test",
    srcs = ["host_network_test.cc"],
    deps = [
        "//googletest:gunit_main",
        "//mcucore/extras/test_tools:print_to_std_string",
        "//mcucore/extras/test_tools:print_value_to_std_string",
        "//mcucore/extras/test_tools:sample_printable",
        "//mcunet/extras/host/ethernet5500:host_network",
    ],
)

cc_test(
    name = "host_socket_info_test",
    srcs = ["host_socket_info_test.cc"],
    deps = [
        "//googletest:gunit_main",
        "//mcucore/extras/test_tools:print_to_std_string",
        "//mcucore/extras/test_tools:print_value_to_std_string",
        "//mcucore/extras/test_tools:sample_printable",
        "//mcunet/extras/host/ethernet5500:host_socket_info",
    ],
)
