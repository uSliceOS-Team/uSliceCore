/**
 * @file test_task_instance.cpp
 * @brief One Entry/Loop/Stop handler triple is reused by multiple tasks.
 * Task objects, each with its own context -- e.g. one motor-control
 * function driving several motors.
 */

#include "test_task_fixture.hpp"
#include "test_scheduler_helpers.hpp"
#include "test_framework.hpp"

struct MotorCtx {
    int speed = 0;
};

TASK_ENTRY(motor) {}

TASK_LOOP(motor) {
    CTX(MotorCtx);
    localTask->speed++;
}

TASK_STOP(motor) {}

TEST_TASK(motor, MotorCtx, true);
constinit MotorCtx motor_rightContext{};
constinit ::uslice::Task motor_right{::uslice::Task::Definition{
    .entry = ::Entry_motor,
    .loop = ::Loop_motor,
    .stop = ::Stop_motor,
    .context = &motor_rightContext,
    .autostart = false,
}};
constexpr ::uslice::TaskLink motorRightLink{&motor_right, nullptr};
constexpr ::uslice::TaskLink motorLink{&motor, &motorRightLink};
constinit const ::uslice::TaskRegistry testRegistry{&motorLink};

int main() {
    // motor_right is configured with autostart=false, so it needs an
    // explicit start.
    CHECK(TASK_RUNNING(motor));
    CHECK(!TASK_RUNNING(motor_right));
    START_TASK(motor_right);

    RUN_PASSES(1); // motor: Entry.        motor_right: Sync (inert)
    RUN_PASSES(1); // motor: first Loop.   motor_right: Entry (staggered by
                   // one pass -- motor_right went through Sync first,
                   // motor didn't need to)

    // Give motor_right's context a different starting point to prove the
    // two instances don't share storage. Safe here: motor_right's Loop
    // hasn't run yet at this point (see above).
    TASK_CONTEXT(motor_right, MotorCtx).speed = 100;

    RUN_PASSES(3); // both now firmly in Loop

    CHECK_EQ(TASK_CONTEXT(motor, MotorCtx).speed, 4); // 1 + 3 more Loop calls
    CHECK_EQ(TASK_CONTEXT(motor_right, MotorCtx).speed,
             103); // 100 + 3 more Loop calls

    return TEST_SUMMARY("test_task_instance");
}
