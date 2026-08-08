/**
 * @file SensorMonitor.cpp
 * @brief Example task: reads a (simulated) sensor, self-stops on a bad
 * reading.
 *
 * Demonstrates the fault model correctly: RAISE_FAULT() only sets a
 * flag, it does NOT stop the task or move it to another case by itself.
 * If a CASE calls RAISE_FAULT() without also calling GOTO_CASE or
 * STOP_SELF(), the task lands right back in the same CASE next pass and
 * raises the fault again, every pass, forever -- that's "just a flag,
 * no action required" working exactly as designed, but it's easy to
 * trip over. The pattern below avoids that: stop yourself in the same
 * CASE where you raise the fault, if that's what you actually want.
 */

#include "tasks/osTaskMacros.hpp"
#include "tasks/osTaskRegMacros.hpp"
#include <cstdio>

CASES(READ, CHECK);

struct SensorCtx {
    int reading = 0;
};

TASK_ENTRY(sensorMonitor) {
    printf("[sensorMonitor] ENTRY handler: sensor warm-up would go here\n");
}

TASK_LOOP(sensorMonitor) {
    CTX(SensorCtx);
    SWITCH
        CASE(READ):
            localTask->reading++;                 // stand-in for ADC_Read()
            GOTO_CASE(CHECK);

        CASE(CHECK):
            if (localTask->reading >= 4) {
                printf("[sensorMonitor] reading %d out of range -- flagging and stopping\n",
                       localTask->reading);
                STOP_SELF();     // decide what happens *before* the jump below
                RAISE_FAULT();   // sets the flag and exits the switch; order
                                  // between these two calls doesn't matter,
                                  // both are plain function calls
            }
            printf("[sensorMonitor] reading %d ok\n", localTask->reading);
            GOTO_CASE(READ);
    SWITCH_END
}

TASK_STOP(sensorMonitor) {
    printf("[sensorMonitor] stopped (cleanup ran)\n");
}

ADD_TASK_AND_START(sensorMonitor, SensorCtx);
