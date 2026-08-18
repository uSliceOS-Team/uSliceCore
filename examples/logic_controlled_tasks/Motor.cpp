/**
 * @file Motor.cpp
 * @brief Example task: spins up to a target speed set from outside.
 *
 * Demonstrates:
 *  - a typed context that Logic configures through the generated API
 *  - manual start selected in Tasks.uslice
 */

#include "TaskDefinitions.hpp"
#include "generated/Tasks.generated.hpp"
#include "tasks/Task.hpp"
#include <iostream>

void Loop_motor(void* rawCtx_, ::uslice::Task* self) {
    auto* localTask = static_cast<MotorContext*>(rawCtx_);
    using enum example::tasks::MotorCase;
    switch (static_cast<example::tasks::MotorCase>(self->currentCase())) {
        case RUN:
            localTask->actualSpeed = localTask->targetSpeed;
            std::cout << "[motor] running at speed=" << localTask->actualSpeed
                      << '\n';
            break;
    }
}

void Stop_motor(void* rawCtx_, [[maybe_unused]] ::uslice::Task* self) {
    const MotorContext* localTask = static_cast<const MotorContext*>(rawCtx_);
    std::cout << "[motor] stopped, last actualSpeed=" << localTask->actualSpeed
              << '\n';
}
