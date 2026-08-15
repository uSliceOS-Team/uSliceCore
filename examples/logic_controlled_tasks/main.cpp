/**
 * @file main.cpp
 * @brief Entry point for the example -- a HOST-SIDE SIMULATION.
 *
 * uSliceCore contains no target-specific code within its supported 32-bit
 * scope and has no build system of its own;
 * on real hardware you would:
 *   1. Wire a 1 kHz hardware timer interrupt to the time module, and
 *   2. Call the generated usliceTaskManager() from the selected main
 *      language; it runs the application registry's supercycle forever.
 *
 * Neither of those fits a runnable console demo (an infinite loop with
 * no output would just hang), so this file does NOT call
 * the infinite manager. It invokes the bounded C++ manager-pass overload for
 * a fixed number of passes so the program can print its final state and exit.
 */

#include "generated/Tasks.generated.hpp"
#include "generated/Manager.generated.hpp"
#include <iostream>

int main(void) {
    std::cout << std::boolalpha
              << "=== uSliceCore example: logic-controlled tasks (host "
                 "simulation) ===\n\n";

    const int PASSES = 12;
    for (int pass = 0; pass < PASSES; pass++) {
        std::cout << "--- pass " << pass << " ---\n";
        usliceTaskManagerPass();
    }

    std::cout << "\n=== final state ===\n"
              << "ledBlinker   running="
              << example::tasks::ledBlinker.isRunning() << '\n'
              << "motor        running=" << example::tasks::motor.isRunning()
              << " actualSpeed=" << example::tasks::motorContext.actualSpeed
              << '\n'
              << "sensorMonitor running="
              << example::tasks::sensorMonitor.isRunning()
              << " faulted=" << example::tasks::sensorMonitor.isFaulted()
              << '\n';

    return 0;
}
