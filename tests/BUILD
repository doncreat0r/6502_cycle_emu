cc_test (
    name = "test_test",
    srcs = glob(["**/*.cc"]),
    copts = ["-Iinclude"],
    deps = [
        "//src:mos6502", "//include:mos6502_headers",
        "@gtest//:gtest_main" 
    ],
)

