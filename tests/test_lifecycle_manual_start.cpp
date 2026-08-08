/**
 * @file test_lifecycle_manual_start.cpp
 * @brief Manual start timing: Stopped -> Guard (inert) -> Entry -> Loop.
 * One extra buffer turn compared to autostart -- see the "Manual/delayed
 * start" workflow in docs/TECHNICAL_REFERENCE.md.
 */

#include "tasks/osTaskCore.hpp"
#include "tasks/osTaskMacros.hpp"
#include "tasks/osTaskRegMacros.hpp"
#include "tasks/osTaskMgmtMacros.hpp"
#include "test_scheduler_helpers.hpp"
#include "test_framework.hpp"

CASES(RUN);

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
    SWITCH
    CASE(RUN) : localTask->loopCount++;
    break;
    SWITCH_END
}

TASK_STOP(manualTask) {}

ADD_TASK(manualTask, ManualCtx); // NOT autostart
DECLARE_TASK(manualTask);

int main() {
    // Before START_TASK: fully stopped, no handler has run, and running
    // the scheduler changes nothing.
    CHECK(!TASK_RUNNING(manualTask));
    RUN_PASSES(3);
    CHECK(!TASK_CONTEXT(manualTask, ManualCtx).entryRan);
    CHECK_EQ(TASK_CONTEXT(manualTask, ManualCtx).loopCount, 0);
    CHECK(!TASK_RUNNING(manualTask));

    START_TASK(manualTask);

    // Turn after START_TASK: Guard, inert. Task reports running (state is
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
