/**
 * @file test_scheduler_helpers.hpp
 * @brief RUN_PASSES(), for tests that drive the scheduler. Separate from
 * test_framework.hpp so tests that don't touch Task at all (test_timer.cpp)
 * don't need to include tasks/osTaskCore.hpp just to get CHECK/CHECK_EQ.
 *
 * Include tasks/osTaskCore.hpp yourself before this header.
 */

#pragma once

/// Runs `count` scheduler passes over every registered task, in order --
/// the same traversal osTaskManager() does internally, just bounded
/// instead of infinite so a test can inspect state between passes and
/// exit.
inline void RUN_PASSES(int count) {
    for (int i = 0; i < count; i++) {
        Task* t = Task::getHead();
        while (t) {
            t = t->execute();
        }
    }
}
