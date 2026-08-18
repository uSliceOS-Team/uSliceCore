/**
 * @file test_lifecycle_autostart.cpp
 * @brief Autostart timing: the first case runs on the first scheduler turn.
 * No startup turn is wasted -- see the
 * "Autostart" workflow in docs/TECHNICAL_REFERENCE.md.
 */

#include "test_task_fixture.hpp"
#include "test_scheduler_helpers.hpp"
#include "test_framework.hpp"

struct AutoCtx {
    int loopCount = 0;
};

void Loop_autoTask(void* rawCtx_, ::uslice::Task* self) {
    auto* localTask = static_cast<AutoCtx*>(rawCtx_);
    switch (self->currentCase()) {
        case 0:
            localTask->loopCount++;
            break;
    }
}

void Stop_autoTask([[maybe_unused]] void* rawCtx_,
                   [[maybe_unused]] ::uslice::Task* self) {}

constexpr ::uslice::Task::Program autoTaskProgram{
    .loop = &Loop_autoTask,
    .stop = &Stop_autoTask,
    .caseCount = 1,
};

constinit AutoCtx autoTaskContext{};
constinit ::uslice::Task autoTask{::uslice::Task::Definition<&autoTaskProgram>{
    .context = &autoTaskContext,
    .autostart = true,
}};
constexpr ::uslice::TaskLink autoTaskLink{&autoTask, nullptr};
constinit const ::uslice::TaskRegistry testRegistry{&autoTaskLink};

int main() {
    // Turn 0: the first case runs immediately.
    RUN_PASSES(1);
    CHECK_EQ(autoTaskContext.loopCount, 1);
    CHECK(autoTask.isRunning());

    // The case keeps running every subsequent turn.
    RUN_PASSES(3);
    CHECK_EQ(autoTaskContext.loopCount, 4);
    CHECK(autoTask.isRunning());

    return TEST_SUMMARY("test_lifecycle_autostart");
}
