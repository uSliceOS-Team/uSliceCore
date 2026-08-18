/**
 * @file test_cross_file_task.cpp
 * @brief Half of test_cross_file_declare_and_context: the task itself,
 * in its own translation unit. See test_cross_file_main.cpp for the
 * other half -- this pair is the actual regression test for the
 * generated-declaration/context pattern used by cross-task control.
 */

#include "test_task_fixture.hpp"
#include "CrossFileCtx.hpp"

void Loop_crossFileMotor(void* rawCtx_, ::uslice::Task* self) {
    auto* localTask = static_cast<CrossFileCtx*>(rawCtx_);
    switch (self->currentCase()) {
        case 0:
            localTask->loopSawTargetSpeed = localTask->targetSpeed;
            localTask->actualSpeed = localTask->targetSpeed;
            break;
    }
}

void Stop_crossFileMotor([[maybe_unused]] void* rawCtx_,
                         [[maybe_unused]] ::uslice::Task* self) {}

constexpr ::uslice::Task::Program crossFileMotorProgram{
    .loop = &Loop_crossFileMotor,
    .stop = &Stop_crossFileMotor,
    .caseCount = 1,
};

constinit CrossFileCtx crossFileMotorContext{};
constinit ::uslice::Task crossFileMotor{
    ::uslice::Task::Definition<&crossFileMotorProgram>{
        .context = &crossFileMotorContext,
        .autostart = false,
    }};
