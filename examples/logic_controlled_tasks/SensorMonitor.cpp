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

#include "TaskDefinitions.hpp"
#include "generated/Tasks.generated.hpp"
#include "tasks/Macros.hpp"
#include <iostream>

TASK_ENTRY(sensorMonitor) {
    std::cout
        << "[sensorMonitor] ENTRY handler: sensor warm-up would go here\n";
}

TASK_LOOP(sensorMonitor) {
    TASK_STATES(READ, CHECK);
    CTX(SensorMonitorContext);
    switch (TASK_STATE()) {
        case READ:
            localTask->reading++; // stand-in for ADC_Read()
            GOTO_CASE(CHECK);

        case CHECK:
            if (localTask->reading >= 4) {
                std::cout << "[sensorMonitor] reading " << localTask->reading
                          << " out of range -- flagging and stopping\n";
                RAISE_FAULT(); // flag only; the return is explicit policy
                STOP_SELF();   // decide what happens *before* the return below
                return;
            }
            std::cout << "[sensorMonitor] reading " << localTask->reading
                      << " ok\n";
            GOTO_CASE(READ);
    }
}

TASK_STOP(sensorMonitor) {
    std::cout << "[sensorMonitor] stopped (cleanup ran)\n";
}
