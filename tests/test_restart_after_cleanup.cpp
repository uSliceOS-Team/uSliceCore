/**
 * @file test_restart_after_cleanup.cpp
 * @brief Restart contract, including start during End and stop during Sync.
 */

#include "test_task_fixture.hpp"
#include "test_framework.hpp"
#include "test_scheduler_helpers.hpp"

struct RestartCtx {
    int loopCount = 0;
    int stopCount = 0;
};

void Loop_restartable(void* rawCtx_, ::uslice::Task* self) {
    auto* localTask = static_cast<RestartCtx*>(rawCtx_);
    switch (self->currentCase()) {
        case 0:
            localTask->loopCount++;
            break;
    }
}

void Stop_restartable(void* rawCtx_, [[maybe_unused]] ::uslice::Task* self) {
    auto* localTask = static_cast<RestartCtx*>(rawCtx_);
    localTask->stopCount++;
}

constexpr ::uslice::Task::Program restartableProgram{
    .loop = &Loop_restartable,
    .stop = &Stop_restartable,
    .caseCount = 1,
};

constinit RestartCtx restartableContext{};
constinit ::uslice::Task restartable{
    ::uslice::Task::Definition<&restartableProgram>{
        .context = &restartableContext,
        .autostart = true,
    }};
constexpr ::uslice::TaskLink restartableLink{&restartable, nullptr};
constinit const ::uslice::TaskRegistry testRegistry{&restartableLink};

int main() {
    RUN_PASSES(1); // Loop

    RestartCtx& ctx = restartableContext;
    CHECK_EQ(ctx.loopCount, 1);

    restartable.stop();
    CHECK(restartable.isRunning());
    CHECK(!restartable.start()); // End is the only rejected start state
    restartable.stop();          // already ending: remains End
    RUN_PASSES(1);               // cleanup
    CHECK_EQ(ctx.stopCount, 1);
    CHECK(!restartable.isRunning());

    CHECK(restartable.start()); // Stopped -> Sync
    restartable.stop();         // stopping is also valid during Sync
    CHECK(!restartable.start());
    RUN_PASSES(1); // cleanup without entering Loop
    CHECK_EQ(ctx.stopCount, 2);
    CHECK_EQ(ctx.loopCount, 1);

    CHECK(restartable.start());
    RUN_PASSES(1); // Sync (inert)
    CHECK_EQ(ctx.loopCount, 1);
    RUN_PASSES(1); // Loop
    CHECK_EQ(ctx.loopCount, 2);

    return TEST_SUMMARY("test_restart_after_cleanup");
}
