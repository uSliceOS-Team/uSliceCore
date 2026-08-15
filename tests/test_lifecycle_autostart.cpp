/**
 * @file test_lifecycle_autostart.cpp
 * @brief Autostart timing: Entry runs on the very first scheduler turn,
 * Loop runs from the turn after that. No wasted turn -- see the
 * "Autostart" workflow in docs/TECHNICAL_REFERENCE.md.
 */

#include "test_task_fixture.hpp"
#include "test_scheduler_helpers.hpp"
#include "test_framework.hpp"

struct AutoCtx {
    bool entryRan = false;
    int loopCount = 0;
};

TASK_ENTRY(autoTask) {
    CTX(AutoCtx);
    localTask->entryRan = true;
}

TASK_LOOP(autoTask) {
    CTX(AutoCtx);
    localTask->loopCount++;
}

TASK_STOP(autoTask) {}

TEST_TASK(autoTask, AutoCtx, true);
constexpr ::uslice::TaskLink autoTaskLink{&autoTask, nullptr};
constinit const ::uslice::TaskRegistry testRegistry{&autoTaskLink};

int main() {
    // Turn 0: Entry runs. Loop has not run yet.
    RUN_PASSES(1);
    CHECK(TASK_CONTEXT(autoTask, AutoCtx).entryRan == true);
    CHECK_EQ(TASK_CONTEXT(autoTask, AutoCtx).loopCount, 0);
    CHECK(TASK_RUNNING(autoTask));

    // Turn 1: first real Loop call.
    RUN_PASSES(1);
    CHECK_EQ(TASK_CONTEXT(autoTask, AutoCtx).loopCount, 1);

    // Loop keeps running every subsequent turn.
    RUN_PASSES(3);
    CHECK_EQ(TASK_CONTEXT(autoTask, AutoCtx).loopCount, 4);
    CHECK(TASK_RUNNING(autoTask));

    return TEST_SUMMARY("test_lifecycle_autostart");
}
