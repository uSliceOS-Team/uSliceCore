/**
 * @file Logic.cpp
 * @brief The task that controls everyone else's lifecycle.
 *
 * Logic is just an ordinary task -- TASK_ENTRY/TASK_LOOP/TASK_STOP
 * like any
 * other. The generated header gives the IDE declarations and exact context
 * types for every configured task.
 */

#include "generated/Tasks.generated.hpp"
#include "tasks/Macros.hpp"
#include "tasks/Task.hpp"
#include <iostream>

TASK_ENTRY(logic) {} // nothing to initialize; still required, empty is fine

TASK_LOOP(logic) {
    TASK_STATES(INIT, MONITOR);
    switch (TASK_STATE()) {
        case INIT:
            std::cout << "[logic] configuring motor and starting it\n";
            example::tasks::motorContext.targetSpeed = 42;
            example::tasks::motor.start();
            GOTO_CASE(MONITOR);

        case MONITOR:
            if (example::tasks::sensorMonitor.isFaulted()) {
                std::cout << "[logic] sensorMonitor faulted at some point "
                             "(diagnostic only)\n";
            }
            break;
    }
}

TASK_STOP(logic) {}
