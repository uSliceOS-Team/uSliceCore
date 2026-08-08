/**
 * @file Motor.cpp
 * @brief Example task: spins up to a target speed set from outside.
 *
 * Demonstrates:
 *  - a context (MotorCtx, see MotorCtx.hpp) that Logic configures via
 *    TASK_CONTEXT before starting this task
 *  - ADD_TASK (not _AND_START): this task starts stopped, waiting for
 *    Logic to set parameters and call START_TASK.
 */

#include "tasks/osTaskMacros.hpp"
#include "tasks/osTaskRegMacros.hpp"
#include "MotorCtx.hpp"
#include <cstdio>

CASES(SPIN_UP);

TASK_ENTRY(motor) {
    CTX(MotorCtx);
    // Logic set targetSpeed via TASK_CONTEXT before calling START_TASK
    // (see Logic.cpp), so it's already here by the time this runs.
    printf("[motor] ENTRY handler: configured targetSpeed=%d\n", localTask->targetSpeed);
}

TASK_LOOP(motor) {
    CTX(MotorCtx);
    SWITCH
        CASE(SPIN_UP):
            localTask->actualSpeed = localTask->targetSpeed;
            printf("[motor] running at speed=%d\n", localTask->actualSpeed);
            break;
    SWITCH_END
}

TASK_STOP(motor) {
    CTX(MotorCtx);
    printf("[motor] stopped, last actualSpeed=%d\n", localTask->actualSpeed);
}

ADD_TASK(motor, MotorCtx);
