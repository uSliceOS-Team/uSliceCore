/**
 * @file Logic.cpp
 * @brief The task that controls everyone else's lifecycle.
 *
 * Logic is just an ordinary task with loop and stop handlers like any
 * other. The generated header gives the IDE declarations and exact context
 * types for every configured task.
 */

#include "generated/Tasks.generated.hpp"
#include "tasks/Task.hpp"
#include <iostream>

void Loop_logic([[maybe_unused]] void* rawCtx_, ::uslice::Task* self) {
    const example::tasks::LogicHandle handle{self};
    using enum example::tasks::LogicCase;
    switch (handle.currentCase()) {
        case INIT:
            std::cout << "[logic] configuring motor and starting it\n";
            example::tasks::motorContext().targetSpeed = 42;
            example::tasks::motor().start();
            handle.gotoCase(MONITOR);
            break;
        case MONITOR:
            if (example::tasks::sensorMonitor().isFaulted()) {
                std::cout << "[logic] sensorMonitor faulted at some point "
                             "(diagnostic only)\n";
            }
            break;
    }
}

void Stop_logic([[maybe_unused]] void* rawCtx_,
                [[maybe_unused]] ::uslice::Task* self) {}
