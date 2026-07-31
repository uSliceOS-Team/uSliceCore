/**
 * @file test_lifecycle_autostart.cpp
 * @brief Autostart timing: Entry runs on the very first scheduler turn,
 * Loop runs from the turn after that. No wasted turn -- see the
 * "Autostart" workflow in the root README's Safe Task Lifecycle.
 */

#include "tasks/osTaskCore.hpp"
#include "tasks/osTaskMacros.hpp"
#include "tasks/osTaskRegMacros.hpp"
#include "tasks/osTaskMgmtMacros.hpp"
#include "test_scheduler_helpers.hpp"
#include "test_framework.hpp"

CASES(RUN);

struct AutoCtx {
  bool entryRan = false;
  int  loopCount = 0;
};

TASK_ENTRY(autoTask) {
  CTX(AutoCtx);
  localTask->entryRan = true;
}

TASK_LOOP(autoTask) {
  CTX(AutoCtx);
  SWITCH
    CASE(RUN):
      localTask->loopCount++;
      break;
  SWITCH_END
}

TASK_STOP(autoTask) {}

ADD_TASK_AND_START(autoTask, AutoCtx);
DECLARE_TASK(autoTask);

int main() {
  // Turn 0: Entry runs. Loop has not run yet.
  RUN_PASSES(1);
  CHECK(TASK_CONTEXT(autoTask, AutoCtx).entryRan == true);
  CHECK_EQ(TASK_CONTEXT(autoTask, AutoCtx).loopCount, 0);
  CHECK(TASK_RUNNING(autoTask));

  // Turn 1: first real Loop call.
  RUN_PASSES(1);
  CHECK_EQ(TASK_CONTEXT(autoTask, AutoCtx).loopCount, 1);

  // Loop keeps running every subsequent turn.
  RUN_PASSES(3);
  CHECK_EQ(TASK_CONTEXT(autoTask, AutoCtx).loopCount, 4);
  CHECK(TASK_RUNNING(autoTask));

  return TEST_SUMMARY("test_lifecycle_autostart");
}
