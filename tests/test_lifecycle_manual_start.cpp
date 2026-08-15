/**
 * @file test_lifecycle_manual_start.cpp
 * @brief Manual start timing: Stopped -> Sync (inert) -> Entry -> Loop.
 * One extra buffer turn compared to autostart -- see the "Manual/delayed
 * start" workflow in docs/TECHNICAL_REFERENCE.md.
 */

#include "test_task_fixture.hpp"
#include "test_scheduler_helpers.hpp"
#include "test_framework.hpp"

struct ManualCtx {
    bool entryRan = false;
    int loopCount = 0;
};

TASK_ENTRY(manualTask) {
    CTX(ManualCtx);
    localTask->entryRan = true;
}

TASK_LOOP(manualTask) {
    CTX(ManualCtx);
    localTask->loopCount++;
}

TASK_STOP(manualTask) {}

TEST_TASK(manualTask, ManualCtx, false);
constexpr ::uslice::TaskLink manualTaskLink{&manualTask, nullptr};
constinit const ::uslice::TaskRegistry testRegistry{&manualTaskLink};

int main() {
    // Before START_TASK: fully stopped, no handler has run, and running
    // the scheduler changes nothing.
    CHECK(!TASK_RUNNING(manualTask));
    RUN_PASSES(3);
    CHECK(!TASK_CONTEXT(manualTask, ManualCtx).entryRan);
    CHECK_EQ(TASK_CONTEXT(manualTask, ManualCtx).loopCount, 0);
    CHECK(!TASK_RUNNING(manualTask));

    START_TASK(manualTask);

    // Turn after START_TASK: Sync, inert. Task reports running (state is
    // no longer Stopped) but Entry has not run yet.
    RUN_PASSES(1);
    CHECK(TASK_RUNNING(manualTask));
    CHECK(!TASK_CONTEXT(manualTask, ManualCtx).entryRan);

    // Next turn: Entry runs. Loop still hasn't.
    RUN_PASSES(1);
    CHECK(TASK_CONTEXT(manualTask, ManualCtx).entryRan);
    CHECK_EQ(TASK_CONTEXT(manualTask, ManualCtx).loopCount, 0);

    // Next turn: first real Loop call.
    RUN_PASSES(1);
    CHECK_EQ(TASK_CONTEXT(manualTask, ManualCtx).loopCount, 1);

    return TEST_SUMMARY("test_lifecycle_manual_start");
}
