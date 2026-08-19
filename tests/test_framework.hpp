/**
 * @file test_framework.hpp
 * @brief Minimal, dependency-free assertions for host-side tests.
 *
 * No external test framework: this is a library for microcontrollers,
 * and pulling in a heavyweight test dependency just to check host-side
 * behavior didn't seem worth it. Each test file is its own small `main()`
 * that steps the scheduler by hand and checks assertions inline; see
 * tests/README.md for why one binary per scenario, not one binary with
 * many independent cases.
 *
 * This header has no dependency on the library itself -- it's usable
 * for any host-side check, task-related or not (see test_timer.cpp,
 * which doesn't touch Task at all). For the RUN_PASSES() scheduler
 * helper, see test_scheduler_helpers.hpp instead.
 */

#pragma once

#include <cstdio>
#include <source_location>

namespace UTest {
inline int g_checks = 0;
inline int g_failures = 0;
} // namespace UTest

namespace UTest {

/// Fails (and records) if `condition` is false, but keeps running the rest of
/// the test -- prefer this by default so one test file reports every failing
/// assertion in a single run, not just the first.
inline void
CHECK(bool condition,
      const std::source_location location = std::source_location::current()) {
    g_checks++;
    if (!condition) {
        std::printf("  FAIL %s:%u: CHECK failed\n", location.file_name(),
                    location.line());
        g_failures++;
    }
}

/// Same as CHECK, but also prints the two values on failure. Requires both
/// sides to be int-convertible -- good enough for the enum/int/bool values
/// these tests compare.
template <typename Actual, typename Expected>
void CHECK_EQ(
    const Actual& actual, const Expected& expected,
    const std::source_location location = std::source_location::current()) {
    g_checks++;
    if (!(actual == expected)) {
        std::printf("  FAIL %s:%u: CHECK_EQ failed -- got %d, expected %d\n",
                    location.file_name(), location.line(),
                    static_cast<int>(actual), static_cast<int>(expected));
        g_failures++;
    }
}

} // namespace UTest

/// Call at the end of main(). Prints a summary and returns the process
/// exit code a shell script (or CI) should propagate: 0 on success,
/// nonzero on any failed CHECK.
inline int TEST_SUMMARY(const char* testName) {
    std::printf("\n%s: %d check(s), %d failure(s)\n", testName, UTest::g_checks,
                UTest::g_failures);
    return UTest::g_failures ? 1 : 0;
}

using UTest::CHECK;
using UTest::CHECK_EQ;
