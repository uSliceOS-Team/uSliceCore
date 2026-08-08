/**
 * @file test_start_stop_noop_windows.cpp
 * @brief START_TASK only takes effect on a fully Stopped task; STOP_TASK
 * only takes effect once a task has actually reached Loop -- calling
 * either outside that window is a silent no-op, not an error. See the
 * note under Task Management in docs/TECHNICAL_REFERENCE.md.
 */

#include "tasks/osTaskCore.hpp"
#include "tasks/osTaskMacros.hpp"
#include "tasks/osTaskRegMacros.hpp"
#include "tasks/osTaskMgmtMacros.hpp"
#include "test_scheduler_helpers.hpp"
#include "test_framework.hpp"

CASES(RUN);

struct Ctx {
    int loopCount = 0;
};

TASK_ENTRY(subject) {}

TASK_LOOP(subject) {
    CTX(Ctx);
    SWITCH
    CASE(RUN) : localTask->loopCount++;
    break;
    SWITCH_END
}

TASK_STOP(subject) {}

ADD_TASK(subject, Ctx); // manual start
DECLARE_TASK(subject);

int main() {
    // STOP_TASK on an already-stopped task: no-op.
    STOP_TASK(subject);
    CHECK(!TASK_RUNNING(subject));

    START_TASK(subject);
    CHECK(TASK_RUNNING(subject)); // Guard: running, but not yet in Loop

    // START_TASK again while already starting: no-op, doesn't restart the
    // Guard/Entry sequence or disturb anything.
    START_TASK(subject);

    // STOP_TASK during Guard: no-op, task keeps starting.
    STOP_TASK(subject);
    RUN_PASSES(1); // consumes the Guard turn
    CHECK(TASK_RUNNING(subject));

    // STOP_TASK during Entry: also a no-op, same reasoning.
    STOP_TASK(subject);
    RUN_PASSES(1); // consumes the Entry turn
    CHECK(TASK_RUNNING(subject));
    CHECK_EQ(TASK_CONTEXT(subject, Ctx).loopCount, 0); // never reached Loop yet

    // Now actually in Loop: STOP_TASK takes effect.
    RUN_PASSES(1);
    CHECK_EQ(TASK_CONTEXT(subject, Ctx).loopCount, 1);
    STOP_TASK(subject);
    CHECK(TASK_RUNNING(subject)); // Stop: running until cleanup actually runs
    RUN_PASSES(1);                // cleanup turn
    CHECK(!TASK_RUNNING(subject));

    // START_TASK again from fully stopped: takes effect, full sequence
    // repeats from Guard. Only currentCase and the fault flag reset on
    // Entry -- loopCount is an ordinary context field, so it keeps
    // whatever value it already had (1) instead of going back to 0.
    START_TASK(subject);
    RUN_PASSES(1); // Guard
    RUN_PASSES(1); // Entry
    RUN_PASSES(1); // first Loop of this second run
    CHECK_EQ(TASK_CONTEXT(subject, Ctx).loopCount, 2); // 1 (carried over) + 1

    return TEST_SUMMARY("test_start_stop_noop_windows");
}
