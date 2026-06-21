load("@rules_cc//cc:cc_test.bzl", "cc_test")

def make_header_map(label_list, strip_prefix="include/"):
    """Helper to create the header map. Use with caution."""
    return {label: label.partition(strip_prefix)[2] for label in label_list}

def cc_header_compilation_test(name, header_label, include_path, srcs = [], deps = [], features = [], timeout = "short", size = "small", copts = [], tags = [], **kwargs):
    """
    Creates a compilation test for one header.

    Args:
        name: Name for the test target.
        srcs: List of additional source files for the test
        deps: Dependencies, like the library being tested, and test utilities
        features: compilation features
        timeout: Test timeout (short, moderate, long, eternal)
        size: Test size (small, medium, large, enormous)
        copts: compiler options
        tags: Tags for the test
        header_label: Bazel label (e.g., //path:file.hpp) for dependency.
        include_path: The exact string to use in the #include "..." directive for the header being tested.
        **kwargs: Additional arguments passed to cc_test
    """
    cpp_file = name + ".cpp"
    native.genrule(
        name = name + "_source",
        outs = [cpp_file],
        # The header path is passed in and used to generate the include line.
        cmd = "echo '#include <%s>\nint main() { return 0; }\n' > $@" % include_path,
        srcs = [header_label],  # This ensures the header is available.
    )

    # native.cc_test(
    cc_test(
        name = name,
        srcs = srcs + [cpp_file],
        deps = deps,
        features = features + ["cxx26_standard"],
        timeout = timeout,
        size = size,
        # copts = copts + ["-nostdlib", "-nostdinc"],
        copts = copts,
        tags = tags,
        **kwargs,
    )

def cc_header_compilation_suite(name, hdrs_map, srcs = [], deps = [], features = [], timeout = "short", size = "small", copts = [], tags = [], **kwargs):
                               
    """
    Creates a test suite of compilation tests for a list of headers.
    
    Args:
        name: Name for the test suite target
        srcs: List of additional source files for the tests
        deps: Dependencies, like the library being tested, and test utilities
        features: compilation features
        timeout: Test timeout (short, moderate, long, eternal)
        size: Test size (small, medium, large, enormous)
        copts: compiler options
        tags: Tags for all tests        
        hdrs_map: a map from header labels to include paths
        **kwargs: Additional arguments passed to cc_test
    """
    test_targets = []
    for header_label, include_path in hdrs_map.items():
        # Create a unique test target name from the header path.
        test_name = name + "--" + include_path.replace("/", "/").replace(".hpp", "").replace(".h", "")
        cc_header_compilation_test(
            name = test_name,
            header_label = header_label,
            include_path = include_path,
            srcs = srcs,
            deps = deps,
            features = features,
            timeout = timeout,
            size = size,
            copts = copts,
            tags = tags,
            **kwargs
        )
        test_targets.append(":" + test_name)
    # Create a test_suite to group all generated tests.
    native.test_suite(
        name = name,
        tests = test_targets,
        tags = tags,
    )
