# Package provides support for decoding HTTP/1.1 requests,
# and (RSN) for creating HTTP/1.1 responses.

load(
    "//mcucore/extras/dev_tools:build_config.bzl",
    "build_config",
)

package(
    default_copts = build_config.default_copts,
)

cc_library(
    name = "request_decoder",
    srcs = ["request_decoder.cc"],
    hdrs = ["request_decoder.h"],
    deps = [
        ":request_decoder_constants",
        "//mcucore/src:hex_escape",
        "//mcucore/src:logging",
        "//mcucore/src:print_misc",
        "//mcucore/src:progmem_string",
        "//mcucore/src:progmem_string_data",
        "//mcucore/src:progmem_string_view",
        "//mcucore/src:string_compare",
        "//mcucore/src:string_view",
    ],
)

cc_library(
    name = "request_decoder_constants",
    srcs = ["request_decoder_constants.cc"],
    hdrs = ["request_decoder_constants.h"],
    deps = [
        "//mcucore/src:flash_string_table",
        "//mcucore/src:mcucore_platform",
        "//mcucore/src:print_misc",
        "//mcucore/src:progmem_string_data",
    ],
)
