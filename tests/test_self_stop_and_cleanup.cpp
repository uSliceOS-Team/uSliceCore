/**
 * @file test_self_stop_and_cleanup.cpp
 * @brief STOP_SELF() and external STOP_TASK() both change state
 * immediately, but cleanup (TASK_STOP) runs on the task's *next*
 * scheduled turn, not the same one -- see Task Lifecycle Timing in
 * docs/TECHNICAL_REFERENCE.md. This test exercises the self-stop path; both share the
 * same underlying taskStop().
 */

#include "tasks/osTaskCore.hpp"
#include "tasks/osTaskMacros.hpp"
#include "tasks/osTaskRegMacros.hpp"
#include "tasks/osTaskMgmtMacros.hpp"
#include "test_scheduler_helpers.hpp"
#include "test_framework.hpp"

CASES(RUN);

struct SelfStopCtx {
    int loopCount = 0;
    bool stopRan = false;
};

TASK_ENTRY(selfStopper) {}

TASK_LOOP(selfStopper) {
    CTX(SelfStopCtx);
    SWITCH
    CASE(RUN) : localTask->loopCount++;
    if (localTask->loopCount == 3) {
        STOP_SELF();
    }
    break;
    SWITCH_END
}

TASK_STOP(selfStopper) {
    CTX(SelfStopCtx);
    localTask->stopRan = true;
}

ADD_TASK_AND_START(selfStopper, SelfStopCtx);
DECLARE_TASK(selfStopper);

int main() {
    RUN_PASSES(1); // Entry
    RUN_PASSES(3); // Loop x3 -- the third call fires STOP_SELF()

    CHECK_EQ(TASK_CONTEXT(selfStopper, SelfStopCtx).loopCount, 3);
    // State changed to Stop already, but cleanup hasn't run yet: this is
    // the same pass STOP_SELF() was called on.
    CHECK(!TASK_CONTEXT(selfStopper, SelfStopCtx).stopRan);
    // isRunning() is true throughout Stop, same as Guard/Entry -- only
    // false once actually Stopped. Don't mistake this turn's TASK_RUNNING
    // for "still fully active."
    CHECK(TASK_RUNNING(selfStopper));

    RUN_PASSES(1); // next scheduled turn: cleanup runs
    CHECK(TASK_CONTEXT(selfStopper, SelfStopCtx).stopRan);
    CHECK(!TASK_RUNNING(selfStopper));

    // Loop must not have been called again after STOP_SELF() -- the task
    // was in Stop, not Loop, on every turn after the third.
    CHECK_EQ(TASK_CONTEXT(selfStopper, SelfStopCtx).loopCount, 3);

    return TEST_SUMMARY("test_self_stop_and_cleanup");
}
