/**
 * @file Logic.cpp
 * @brief The task that controls everyone else's lifecycle.
 *
 * Logic is just an ordinary task -- CASES/TASK_ENTRY/TASK_LOOP/TASK_STOP
 * like any
 * other -- the only thing setting it apart is that its .cpp includes
 * Registrations.hpp and therefore is allowed to call START_TASK /
 * STOP_TASK / TASK_RUNNING / TASK_FAULTED / TASK_CONTEXT on tasks by
 * name. Nothing in the framework enforces that convention; it's just
 * the one file in the project that happens to reach across.
 */

#include "tasks/osTaskMacros.hpp"
#include "tasks/osTaskRegMacros.hpp"
#include "Registrations.hpp"
#include <cstdio>

CASES(INIT, MONITOR);

struct LogicCtx {};

TASK_ENTRY(logic) {}  // nothing to initialize; still required, empty is fine

TASK_LOOP(logic) {
    CTX(LogicCtx);
    SWITCH
        CASE(INIT):
            printf("[logic] configuring motor and starting it\n");
            TASK_CONTEXT(motor, MotorCtx).targetSpeed = 42;
            START_TASK(motor);
            GOTO_CASE(MONITOR);

        CASE(MONITOR):
            if (TASK_FAULTED(sensorMonitor)) {
                printf("[logic] sensorMonitor faulted at some point (diagnostic only)\n");
            }
            break;
    SWITCH_END
}

TASK_STOP(logic) {}

ADD_TASK_AND_START(logic, LogicCtx);
