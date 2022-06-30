cc_library(
    name = "olcPixelGameEngine",
    hdrs = ["olcPixelGameEngine/olcPixelGameEngine.h"],
)

cc_binary(
    name = "mainTestCPU",
    srcs = ["mainTestCPU.cpp"],
    copts = ["-Iinclude"], # TODO: build new cpp toolchain with the following options "-lX11", "-lGL", "-lpthread", "-lpng", "-lstdc++fs", "-std=c++14",
    deps = ["//src:mos6502", "//include:mos6502_headers", "//:olcPixelGameEngine"]
)
