/**
 * @file test_fault_flag.cpp
 * @brief RAISE_FAULT() only sets a flag -- it does not stop the task and
 * does not move it to another CASE. Two tasks here:
 *  - naiveFaulter: raises a fault without ever transitioning away, so it
 *    re-raises every single pass, forever. This is "just a flag, no
 *    action required" working exactly as documented, not a bug -- but
 *    it's the trap called out in Correct and Incorrect Patterns, and
 *    this test pins down that it really does behave this way rather
 *    than relying on a written description of it.
 *  - safeFaulter: raises a fault AND calls STOP_SELF() in the same
 *    CASE, the documented correct pattern.
 */

#include "tasks/osTaskCore.hpp"
#include "tasks/osTaskMacros.hpp"
#include "tasks/osTaskRegMacros.hpp"
#include "tasks/osTaskMgmtMacros.hpp"
#include "test_scheduler_helpers.hpp"
#include "test_framework.hpp"

// --- naiveFaulter: the trap ---

CASES(NAIVE_CHECK);

struct NaiveCtx {
    int loopCount = 0;
};

TASK_ENTRY(naiveFaulter) {}

TASK_LOOP(naiveFaulter) {
    CTX(NaiveCtx);
    SWITCH
    CASE(NAIVE_CHECK) : localTask->loopCount++;
    RAISE_FAULT(); // no GOTO_CASE, no STOP_SELF -- lands right back here
    SWITCH_END
}

TASK_STOP(naiveFaulter) {}

ADD_TASK_AND_START(naiveFaulter, NaiveCtx);
DECLARE_TASK(naiveFaulter);

// --- safeFaulter: the documented correct pattern ---

CASES(SAFE_READ, SAFE_CHECK);

struct SafeCtx {
    int reading = 0;
};

TASK_ENTRY(safeFaulter) {}

TASK_LOOP(safeFaulter) {
    CTX(SafeCtx);
    SWITCH
    CASE(SAFE_READ) : localTask->reading++;
    GOTO_CASE(SAFE_CHECK);
    CASE(SAFE_CHECK) : if (localTask->reading >= 2) {
        STOP_SELF();
        RAISE_FAULT();
    }
    GOTO_CASE(SAFE_READ);
    SWITCH_END
}

TASK_STOP(safeFaulter) {}

ADD_TASK_AND_START(safeFaulter, SafeCtx);
DECLARE_TASK(safeFaulter);

int main() {
    RUN_PASSES(1); // Entry for both

    // naiveFaulter: every pass raises the fault again, loopCount keeps
    // climbing, and the task never stops on its own.
    RUN_PASSES(5);
    CHECK_EQ(TASK_CONTEXT(naiveFaulter, NaiveCtx).loopCount, 5);
    CHECK(TASK_FAULTED(naiveFaulter));
    CHECK(TASK_RUNNING(naiveFaulter)); // still running -- nothing stopped it

    // safeFaulter: READ/CHECK alternate one CASE per turn, so reaching
    // reading==2 and stopping takes a few turns; five is enough headroom.
    CHECK(TASK_FAULTED(safeFaulter));
    CHECK(!TASK_RUNNING(safeFaulter)); // actually stopped, unlike naiveFaulter

    return TEST_SUMMARY("test_fault_flag");
}
