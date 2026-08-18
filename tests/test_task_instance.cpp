/**
 * @file test_task_instance.cpp
 * @brief One task Program is reused by multiple task objects.
 * Task objects, each with its own context -- e.g. one motor-control
 * function driving several motors.
 */

#include "test_task_fixture.hpp"
#include "test_scheduler_helpers.hpp"
#include "test_framework.hpp"

struct MotorCtx {
    int speed = 0;
};

void Loop_motor(void* rawCtx_, ::uslice::Task* self) {
    auto* localTask = static_cast<MotorCtx*>(rawCtx_);
    switch (self->currentCase()) {
        case 0:
            localTask->speed++;
            break;
    }
}

void Stop_motor([[maybe_unused]] void* rawCtx_,
                [[maybe_unused]] ::uslice::Task* self) {}

constexpr ::uslice::Task::Program motorProgram{
    .loop = &Loop_motor,
    .stop = &Stop_motor,
    .caseCount = 1,
};

constinit MotorCtx motorContext{};
constinit ::uslice::Task motor{::uslice::Task::Definition<&motorProgram>{
    .context = &motorContext,
    .autostart = true,
}};
constinit MotorCtx motor_rightContext{};
constinit ::uslice::Task motor_right{::uslice::Task::Definition<&motorProgram>{
    .context = &motor_rightContext,
    .autostart = false,
}};
constexpr ::uslice::TaskLink motorRightLink{&motor_right, nullptr};
constexpr ::uslice::TaskLink motorLink{&motor, &motorRightLink};
constinit const ::uslice::TaskRegistry testRegistry{&motorLink};

int main() {
    // motor_right is configured with autostart=false, so it needs an
    // explicit start.
    CHECK(motor.isRunning());
    CHECK(!motor_right.isRunning());
    CHECK(motor_right.start());

    RUN_PASSES(1); // motor: first Loop. motor_right: Sync (inert)

    // Give motor_right's context a different starting point to prove the
    // two instances don't share storage. Safe here: motor_right's case
    // hasn't run yet at this point (see above).
    motor_rightContext.speed = 100;

    RUN_PASSES(3); // both now dispatching their case

    CHECK_EQ(motorContext.speed, 4); // 1 + 3 more Loop calls
    CHECK_EQ(motor_rightContext.speed,
             103); // 100 + 3 more case calls

    return TEST_SUMMARY("test_task_instance");
}
