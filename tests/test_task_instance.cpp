/**
 * @file test_task_instance.cpp
 * @brief TASK_INSTANCE reuses one Entry/Loop/Stop triple across multiple
 * Task objects, each with its own context -- e.g. one motor-control
 * function driving several motors.
 */

#include "tasks/osTaskCore.hpp"
#include "tasks/osTaskMacros.hpp"
#include "tasks/osTaskRegMacros.hpp"
#include "tasks/osTaskMgmtMacros.hpp"
#include "test_scheduler_helpers.hpp"
#include "test_framework.hpp"

CASES(SPIN);

struct MotorCtx {
  int speed = 0;
};

TASK_ENTRY(motor) {}

TASK_LOOP(motor) {
  CTX(MotorCtx);
  SWITCH
    CASE(SPIN):
      localTask->speed++;
      break;
  SWITCH_END
}

TASK_STOP(motor) {}

ADD_TASK_AND_START(motor, MotorCtx);
TASK_INSTANCE(motor_right, motor, MotorCtx);  // shares Entry_motor/Loop_motor/Stop_motor

DECLARE_TASK(motor);
DECLARE_TASK(motor_right);

int main() {
  // motor_right was registered via TASK_INSTANCE with autostart=false
  // (REGISTER_TASK's default), so it needs an explicit start.
  CHECK(TASK_RUNNING(motor));
  CHECK(!TASK_RUNNING(motor_right));
  START_TASK(motor_right);

  RUN_PASSES(1);  // motor: Entry.        motor_right: Guard (inert)
  RUN_PASSES(1);  // motor: first Loop.   motor_right: Entry (staggered by
                   // one pass -- motor_right went through Guard first,
                   // motor didn't need to)

  // Give motor_right's context a different starting point to prove the
  // two instances don't share storage. Safe here: motor_right's Loop
  // hasn't run yet at this point (see above).
  TASK_CONTEXT(motor_right, MotorCtx).speed = 100;

  RUN_PASSES(3);  // both now firmly in Loop

  CHECK_EQ(TASK_CONTEXT(motor, MotorCtx).speed, 4);        // 1 + 3 more Loop calls
  CHECK_EQ(TASK_CONTEXT(motor_right, MotorCtx).speed, 103); // 100 + 3 more Loop calls

  return TEST_SUMMARY("test_task_instance");
}
