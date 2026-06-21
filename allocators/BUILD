load("@rules_cc//cc:cc_library.bzl", "cc_library")

HEADERS = glob(["include/**/*.hpp"])
STRIP_INCLUDE_PREFIX = "include/"

cc_library(
    name = "allocators",
    hdrs = HEADERS,
    visibility = ["//visibility:public"],
    strip_include_prefix = STRIP_INCLUDE_PREFIX
)

load("//bazel:header_compilation_tests.bzl", "cc_header_compilation_suite", "make_header_map")
cc_header_compilation_suite(
    name = "header_compiles",
    hdrs_map = make_header_map(HEADERS, STRIP_INCLUDE_PREFIX),
    deps = [":allocators"],
    tags = ["header-compilation-test"]
)
