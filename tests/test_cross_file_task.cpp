/**
 * @file test_cross_file_task.cpp
 * @brief Half of test_cross_file_declare_and_context: the task itself,
 * in its own translation unit. See test_cross_file_main.cpp for the
 * other half -- this pair is the actual regression test for the
 * generated-declaration/context pattern used by cross-task control.
 */

#include "test_task_fixture.hpp"
#include "CrossFileCtx.hpp"

TASK_ENTRY(crossFileMotor) {
    CTX(CrossFileCtx);
    localTask->entrySawTargetSpeed = localTask->targetSpeed;
}

TASK_LOOP(crossFileMotor) {
    CTX(CrossFileCtx);
    localTask->actualSpeed = localTask->targetSpeed;
}

TASK_STOP(crossFileMotor) {}

TEST_TASK(crossFileMotor, CrossFileCtx, false);
