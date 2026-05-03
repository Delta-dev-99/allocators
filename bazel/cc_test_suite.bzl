load("@rules_cc//cc:cc_test.bzl", "cc_test")

def cc_test_suite(name, srcs, deps = [], features = [], timeout = None, size = None, copts = [], tags = [], strip_test_prefix = "tests/", **kwargs):
    """
    Creates a cc_test for each source file and a test_suite that groups them.

    Args:
        name: Name of the test suite
        srcs: List of source files (each becomes a separate test)
        deps: Dependencies for all tests
        features: Compiler features for all tests
        timeout: Test timeout (short, moderate, long, eternal)
        size: Test size (small, medium, large, enormous)
        copts: compiler options
        tags: Tags for all tests
        strip_test_prefix: Prefix to strip from test source paths when creating test names
        **kwargs: Additional arguments passed to each cc_test
    """
    
    test_targets = []

    for src in srcs:
        # Generate test name from source file path
        # Remove common prefix and file extension, replace slashes with underscores
        test_name = src

        if strip_test_prefix and test_name.startswith(strip_test_prefix):
            test_name = test_name[len(strip_test_prefix):]
        
        # Remove .cpp extension
        if test_name.endswith(".cpp"):
            test_name = test_name[:-4]
        elif test_name.endswith(".cc"):
            test_name = test_name[:-3]
        
        # Replace path separators with underscores
        test_name = test_name.replace("/", "_").replace("\\", "_")

        # Prepend suite name to avoid conflicts
        full_test_name = "{}_{}".format(name, test_name)

        test_targets.append(":" + full_test_name)

        cc_test(
            name = full_test_name,
            srcs = [src],
            deps = deps + ["//test_support"],
            features = features,
            timeout = timeout,
            size = size,
            copts = copts + [
                # "-Weverything",
                "-Weffc++",
                "-Wall",
                # "-Wno-c++98-compat",
            ],
            tags = tags,
            **kwargs
        )

    # Create test suite that groups all individual tests
    native.test_suite(
        name = name,
        tests = test_targets,
        tags = tags,
    )

