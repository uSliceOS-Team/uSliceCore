/**
 * @file test_cross_file_task.cpp
 * @brief Half of test_cross_file_declare_and_context: the task itself,
 * in its own translation unit. See test_cross_file_main.cpp for the
 * other half -- this pair is the actual regression test for the
 * DECLARE_TASK / TASK_CONTEXT pattern in Task Lifecycle Control, the
 * exact mechanism this whole test suite exists because of getting
 * wrong once already during development.
 */

#include "tasks/osTaskMacros.hpp"
#include "tasks/osTaskRegMacros.hpp"
#include "CrossFileCtx.hpp"

CASES(SPIN_UP);

TASK_ENTRY(crossFileMotor) {
  CTX(CrossFileCtx);
  localTask->entrySawTargetSpeed = localTask->targetSpeed;
}

TASK_LOOP(crossFileMotor) {
  CTX(CrossFileCtx);
  SWITCH
    CASE(SPIN_UP):
      localTask->actualSpeed = localTask->targetSpeed;
      break;
  SWITCH_END
}

TASK_STOP(crossFileMotor) {}

ADD_TASK(crossFileMotor, CrossFileCtx);  // manual start: main.cpp configures then starts
