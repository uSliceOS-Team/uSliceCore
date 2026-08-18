/**
 * @file test_lifecycle_manual_start.cpp
 * @brief Manual start timing: Stopped -> Sync (inert) -> Loop.
 * One extra buffer turn compared to autostart -- see the "Manual/delayed
 * start" workflow in docs/TECHNICAL_REFERENCE.md.
 */

#include "test_task_fixture.hpp"
#include "test_scheduler_helpers.hpp"
#include "test_framework.hpp"

struct ManualCtx {
    int loopCount = 0;
};

void Loop_manualTask(void* rawCtx_, ::uslice::Task* self) {
    auto* localTask = static_cast<ManualCtx*>(rawCtx_);
    switch (self->currentCase()) {
        case 0:
            localTask->loopCount++;
            break;
    }
}

void Stop_manualTask([[maybe_unused]] void* rawCtx_,
                     [[maybe_unused]] ::uslice::Task* self) {}

constexpr ::uslice::Task::Program manualTaskProgram{
    .loop = &Loop_manualTask,
    .stop = &Stop_manualTask,
    .caseCount = 1,
};

constinit ManualCtx manualTaskContext{};
constinit ::uslice::Task manualTask{
    ::uslice::Task::Definition<&manualTaskProgram>{
        .context = &manualTaskContext,
        .autostart = false,
    }};
constexpr ::uslice::TaskLink manualTaskLink{&manualTask, nullptr};
constinit const ::uslice::TaskRegistry testRegistry{&manualTaskLink};

int main() {
    // Before start(): fully stopped, no handler has run, and running
    // the scheduler changes nothing.
    CHECK(!manualTask.isRunning());
    RUN_PASSES(3);
    CHECK_EQ(manualTaskContext.loopCount, 0);
    CHECK(!manualTask.isRunning());

    CHECK(manualTask.start());
    CHECK(manualTask.start()); // already starting: accepted no-op

    // Turn after start(): Sync, inert. Task reports running (state is
    // no longer Stopped) but Loop has not run yet.
    RUN_PASSES(1);
    CHECK(manualTask.isRunning());
    CHECK_EQ(manualTaskContext.loopCount, 0);
    CHECK(manualTask.start()); // already running: accepted no-op

    // Next turn: first Loop call.
    RUN_PASSES(1);
    CHECK_EQ(manualTaskContext.loopCount, 1);

    return TEST_SUMMARY("test_lifecycle_manual_start");
}
