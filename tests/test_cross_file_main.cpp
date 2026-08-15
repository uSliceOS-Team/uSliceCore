/**
 * @file test_cross_file_main.cpp
 * @brief Half of test_cross_file_declare_and_context: reaches the task
 * defined in test_cross_file_task.cpp through public declarations, then
 * configures its context before starting it and confirms the value survives
 * the Sync buffer turn and is visible inside the task's own Entry handler
 * -- this is the exact mechanism from Task Lifecycle Control in the
 * technical reference, exercised for real instead of just described.
 */

#include "tasks/Task.hpp"
#include "CrossFileCtx.hpp"
#include "test_scheduler_helpers.hpp"
#include "test_framework.hpp"

extern ::uslice::Task crossFileMotor;
extern CrossFileCtx crossFileMotorContext;

constexpr ::uslice::TaskLink crossFileMotorLink{&crossFileMotor, nullptr};
constinit const ::uslice::TaskRegistry testRegistry{&crossFileMotorLink};

#define TASK_CONTEXT(name, ContextType) (name##Context)
#define START_TASK(name) (name.start())

int main() {
    TASK_CONTEXT(crossFileMotor, CrossFileCtx).targetSpeed = 77;
    START_TASK(crossFileMotor);

    RUN_PASSES(1); // Sync: inert, targetSpeed must still read back as 77
    CHECK_EQ(TASK_CONTEXT(crossFileMotor, CrossFileCtx).targetSpeed, 77);

    RUN_PASSES(1); // Entry: the task's own Entry_ handler runs now
    CHECK_EQ(TASK_CONTEXT(crossFileMotor, CrossFileCtx).entrySawTargetSpeed,
             77);

    RUN_PASSES(1); // first real Loop
    CHECK_EQ(TASK_CONTEXT(crossFileMotor, CrossFileCtx).actualSpeed, 77);

    return TEST_SUMMARY("test_cross_file_declare_and_context");
}
