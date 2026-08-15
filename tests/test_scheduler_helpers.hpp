/**
 * @file test_scheduler_helpers.hpp
 * @brief RUN_PASSES(), for tests that drive the scheduler. Separate from
 * test_framework.hpp so tests that don't touch Task at all (test_timer.cpp)
 * don't need to include tasks/Task.hpp just to get CHECK/CHECK_EQ.
 *
 * Include tasks/Task.hpp yourself before this header.
 */

#pragma once

/// Runs `count` scheduler passes over every registered task, in order --
/// the same traversal as the explicitly configured scheduler entry point,
/// just bounded instead of infinite so a test can inspect state between
/// passes and exit.
inline void runPasses(const ::uslice::TaskRegistry& registry, int count) {
    for (int i = 0; i < count; i++) {
        registry.executePass();
    }
}

#define RUN_PASSES(count) runPasses(testRegistry, count)
