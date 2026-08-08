/**
 * @file test_framework.hpp
 * @brief Minimal, dependency-free assertion macros for host-side tests.
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

namespace UTest {
inline int g_checks = 0;
inline int g_failures = 0;
} // namespace UTest

/// Fails (and records) if `cond` is false, but keeps running the rest of
/// the test -- prefer this by default so one test file reports every
/// failing assertion in a single run, not just the first.
#define CHECK(cond)                                                            \
    do {                                                                       \
        UTest::g_checks++;                                                     \
        if (!(cond)) {                                                         \
            std::printf("  FAIL %s:%d: CHECK(%s)\n", __FILE__, __LINE__,       \
                        #cond);                                                \
            UTest::g_failures++;                                               \
        }                                                                      \
    } while (0)

/// Same as CHECK, but also prints the two values on failure. Requires
/// both sides to be printable with %d (int-convertible) -- good enough
/// for the enum/int/bool values these tests compare.
#define CHECK_EQ(actual, expected)                                             \
    do {                                                                       \
        UTest::g_checks++;                                                     \
        auto av_ = (actual);                                                   \
        auto ev_ = (expected);                                                 \
        if (!(av_ == ev_)) {                                                   \
            std::printf(                                                       \
                "  FAIL %s:%d: CHECK_EQ(%s, %s) -- got %d, expected %d\n",     \
                __FILE__, __LINE__, #actual, #expected, static_cast<int>(av_), \
                static_cast<int>(ev_));                                        \
            UTest::g_failures++;                                               \
        }                                                                      \
    } while (0)

/// Call at the end of main(). Prints a summary and returns the process
/// exit code a shell script (or CI) should propagate: 0 on success,
/// nonzero on any failed CHECK.
inline int TEST_SUMMARY(const char* testName) {
    std::printf("\n%s: %d check(s), %d failure(s)\n", testName, UTest::g_checks,
                UTest::g_failures);
    return UTest::g_failures ? 1 : 0;
}
