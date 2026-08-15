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
#include "tasks/Macros.hpp"
#include <iostream>

TASK_ENTRY(motor) {
    CTX(const MotorContext);
    // Logic used the generated typed context before calling start(), so the
    // value is already visible by the time this handler runs.
    std::cout << "[motor] ENTRY handler: configured targetSpeed="
              << localTask->targetSpeed << '\n';
}

TASK_LOOP(motor) {
    CTX(MotorContext);
    localTask->actualSpeed = localTask->targetSpeed;
    std::cout << "[motor] running at speed=" << localTask->actualSpeed << '\n';
}

TASK_STOP(motor) {
    CTX(const MotorContext);
    std::cout << "[motor] stopped, last actualSpeed=" << localTask->actualSpeed
              << '\n';
}
