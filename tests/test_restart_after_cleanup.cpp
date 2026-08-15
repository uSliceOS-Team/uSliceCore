/**
 * @file test_restart_after_cleanup.cpp
 * @brief Supported stop-wait-start lifecycle without relying on rejected
 * lifecycle commands as control flow.
 */

#include "test_task_fixture.hpp"
#include "test_framework.hpp"
#include "test_scheduler_helpers.hpp"

struct RestartCtx {
    int entryCount = 0;
    int loopCount = 0;
    int stopCount = 0;
};

TASK_ENTRY(restartable) {
    CTX(RestartCtx);
    localTask->entryCount++;
}

TASK_LOOP(restartable) {
    CTX(RestartCtx);
    localTask->loopCount++;
}

TASK_STOP(restartable) {
    CTX(RestartCtx);
    localTask->stopCount++;
}

TEST_TASK(restartable, RestartCtx, true);
constexpr ::uslice::TaskLink restartableLink{&restartable, nullptr};
constinit const ::uslice::TaskRegistry testRegistry{&restartableLink};

int main() {
    RUN_PASSES(1); // Entry
    RUN_PASSES(1); // Loop

    RestartCtx& ctx = TASK_CONTEXT(restartable, RestartCtx);
    CHECK_EQ(ctx.entryCount, 1);
    CHECK_EQ(ctx.loopCount, 1);

    STOP_TASK(restartable); // legal in Loop
    CHECK(TASK_RUNNING(restartable));
    RUN_PASSES(1); // cleanup
    CHECK_EQ(ctx.stopCount, 1);
    CHECK(!TASK_RUNNING(restartable));

    START_TASK(restartable); // legal only after fully Stopped
    RUN_PASSES(1);           // Sync (inert)
    CHECK_EQ(ctx.entryCount, 1);
    CHECK_EQ(ctx.loopCount, 1);
    RUN_PASSES(1); // Entry
    CHECK_EQ(ctx.entryCount, 2);
    CHECK_EQ(ctx.loopCount, 1);
    RUN_PASSES(1); // Loop
    CHECK_EQ(ctx.loopCount, 2);

    return TEST_SUMMARY("test_restart_after_cleanup");
}
