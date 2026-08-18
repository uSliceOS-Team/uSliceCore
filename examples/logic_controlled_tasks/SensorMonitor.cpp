/**
 * @file SensorMonitor.cpp
 * @brief Example task: reads a (simulated) sensor, self-stops on a bad
 * reading.
 *
 * Demonstrates the fault model correctly: raiseFault() only sets a
 * flag, it does NOT stop the task or move it to another state by itself.
 * If a case calls raiseFault() without also calling gotoCase() or stop(), the
 * task runs that same case again on the next pass and
 * raises the fault again, every pass, forever -- that's "just a flag,
 * no action required" working exactly as designed, but it's easy to
 * trip over. The pattern below avoids that: stop yourself in the same
 * switch branch where you raise the fault, if that's what you actually want.
 */

#include "TaskDefinitions.hpp"
#include "generated/Tasks.generated.hpp"
#include "tasks/Task.hpp"
#include <iostream>

void Loop_sensorMonitor(void* rawCtx_, ::uslice::Task* self) {
    auto* localTask = static_cast<SensorMonitorContext*>(rawCtx_);
    using enum example::tasks::SensorMonitorCase;
    switch (
        static_cast<example::tasks::SensorMonitorCase>(self->currentCase())) {
        case READ:
            localTask->reading++; // stand-in for ADC_Read()
            self->gotoCase(static_cast<::uslice::Task::case_t>(CHECK));
            break;
        case CHECK:
            if (localTask->reading >= 4) {
                std::cout << "[sensorMonitor] reading " << localTask->reading
                          << " out of range -- flagging and stopping\n";
                self->raiseFault(); // flag only; stop is explicit policy
                self->stop();
                break;
            }
            std::cout << "[sensorMonitor] reading " << localTask->reading
                      << " ok\n";
            self->gotoCase(static_cast<::uslice::Task::case_t>(READ));
            break;
    }
}

void Stop_sensorMonitor([[maybe_unused]] void* rawCtx_,
                        [[maybe_unused]] ::uslice::Task* self) {
    std::cout << "[sensorMonitor] stopped (cleanup ran)\n";
}
