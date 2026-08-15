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

#include "test_task_fixture.hpp"
#include "test_scheduler_helpers.hpp"
#include "test_framework.hpp"

// --- naiveFaulter: the trap ---

struct NaiveCtx {
    int loopCount = 0;
};

TASK_ENTRY(naiveFaulter) {}

TASK_LOOP(naiveFaulter) {
    CTX(NaiveCtx);
    localTask->loopCount++;
    RAISE_FAULT(); // no lifecycle or control-flow effect
}

TASK_STOP(naiveFaulter) {}

TEST_TASK(naiveFaulter, NaiveCtx, true);

// --- safeFaulter: the documented correct pattern ---

struct SafeCtx {
    int reading = 0;
};

TASK_ENTRY(safeFaulter) {}

TASK_LOOP(safeFaulter) {
    TASK_STATES(READ, CHECK);
    CTX(SafeCtx);
    switch (TASK_STATE()) {
        case READ:
            localTask->reading++;
            GOTO_CASE(CHECK);
        case CHECK:
            if (localTask->reading >= 2) {
                STOP_SELF();
                RAISE_FAULT();
                return;
            }
            GOTO_CASE(READ);
    }
}

TASK_STOP(safeFaulter) {}

TEST_TASK(safeFaulter, SafeCtx, true);

constexpr ::uslice::TaskLink safeFaulterLink{&safeFaulter, nullptr};
constexpr ::uslice::TaskLink naiveFaulterLink{&naiveFaulter, &safeFaulterLink};
constinit const ::uslice::TaskRegistry testRegistry{&naiveFaulterLink};

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
