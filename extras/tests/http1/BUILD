cc_test(
    name = "request_decoder_internals_test",
    srcs = ["request_decoder_internals_test.cc"],
    deps = [
        "//googletest:gunit_main",
        "//mcunet/src/http1:request_decoder",
    ],
)

cc_test(
    name = "request_decoder_test",
    srcs = ["request_decoder_test.cc"],
    deps = [
        "//absl/strings",
        "//googletest:gunit_main",
        "//mcucore/extras/test_tools:print_to_std_string",
        "//mcucore/extras/test_tools:print_value_to_std_string",
        "//mcucore/extras/test_tools:progmem_string_utils",
        "//mcucore/extras/test_tools:sample_printable",
        "//mcucore/src:string_view",
        "//mcunet/extras/test_tools:mock_request_decoder_listener",
        "//mcunet/extras/test_tools:string_utils",
        "//mcunet/src/http1:request_decoder",
        "//mcunet/src/http1:request_decoder_constants",
    ],
)
