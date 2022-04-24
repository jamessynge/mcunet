cc_test(
    name = "ethernet_address_test",
    srcs = ["ethernet_address_test.cc"],
    deps = [
        "//googletest:gunit_main",
        "//mcucore/extras/test_tools:print_value_to_std_string",
        "//mcucore/extras/test_tools:status_test_utils",
        "//mcucore/src:eeprom_region",
        "//mcunet/src:ethernet_address",
    ],
)

cc_test(
    name = "ip_address_test",
    srcs = ["ip_address_test.cc"],
    deps = [
        "//googletest:gunit_main",
        "//mcucore/extras/test_tools:print_value_to_std_string",
        "//mcucore/extras/test_tools:status_test_utils",
        "//mcunet/src:ip_address",
    ],
)

cc_test(
    name = "server_socket_test",
    srcs = ["server_socket_test.cc"],
    deps = [
        "//googletest:gunit_main",
        "//mcunet/extras/test_tools:mock_socket_listener",
        "//mcunet/src:server_socket",
    ],
)
