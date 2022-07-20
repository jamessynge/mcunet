cc_test(
    name = "addresses_test",
    srcs = ["addresses_test.cc"],
    deps = [
        "//googletest:gunit_main",
        "//mcucore/extras/host/eeprom",
        "//mcucore/extras/test_tools:print_value_to_std_string",
        "//mcucore/extras/test_tools:status_test_utils",
        "//mcucore/src:eeprom_region",
        "//mcucore/src:eeprom_tlv",
        "//mcucore/src:status_code",
        "//mcunet/src:addresses",
        "//mcunet/src:ethernet_address",
        "//mcunet/src:ip_address",
    ],
)

cc_test(
    name = "ethernet_address_test",
    srcs = ["ethernet_address_test.cc"],
    deps = [
        "//googletest:gunit_main",
        "//mcucore/extras/test_tools:print_value_to_std_string",
        "//mcucore/extras/test_tools:status_test_utils",
        "//mcucore/src:eeprom_region",
        "//mcucore/src:mcucore_platform",
        "//mcunet/src:ethernet_address",
    ],
)

cc_test(
    name = "ip_address_test",
    srcs = ["ip_address_test.cc"],
    deps = [
        "//absl/strings",
        "//googletest:gunit_main",
        "//mcucore/extras/test_tools:print_value_to_std_string",
        "//mcucore/extras/test_tools:status_test_utils",
        "//mcucore/src:eeprom_region",
        "//mcucore/src:status",
        "//mcunet/src:ip_address",
        "//mcunet/src:platform_network",
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

cc_test(
    name = "write_buffered_connection_test",
    srcs = ["write_buffered_connection_test.cc"],
    deps = [
        "//googletest:gunit_main",
        "//mcunet/extras/test_tools:fake_write_buffered_connection",
        "//mcunet/extras/test_tools:mock_client",
        "//mcunet/src:write_buffered_connection",
    ],
)
