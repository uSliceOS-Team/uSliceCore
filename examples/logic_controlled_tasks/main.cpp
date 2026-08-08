/**
 * @file main.cpp
 * @brief Entry point for the example -- a HOST-SIDE SIMULATION.
 *
 * uSliceCore is platform-independent and has no build system of its own;
 * on real hardware you would:
 *   1. Wire a 1 kHz hardware timer interrupt to osTickISR() (or
 *      OS::Time::Core::onTickISR() from C++), and
 *   2. Call osTaskManager() once from main() -- it runs the supercycle
 *      forever and never returns.
 *
 * Neither of those fits a runnable console demo (an infinite loop with
 * no output would just hang), so this file does NOT call
 * osTaskManager(). Instead it walks the task list by hand, exactly the
 * way osTaskManager() does internally (see tasks/osTaskManager.cpp),
 * for a fixed number of passes so you can see the output and the
 * program can exit. This unrolling is specific to this demo, not part
 * of the library.
 */

#include "tasks/osTaskCore.hpp"
#include "Registrations.hpp"
#include <cstdio>

int main(void) {
    printf("=== uSliceCore example: logic-controlled tasks (host simulation) "
           "===\n\n");

    const int PASSES = 12;
    for (int pass = 0; pass < PASSES; pass++) {
        printf("--- pass %d ---\n", pass);
        Task* t = Task::getHead();
        while (t) {
            t = t->execute();
        }
    }

    printf("\n=== final state ===\n");
    printf("ledBlinker   running=%d\n", TASK_RUNNING(ledBlinker));
    printf("motor        running=%d actualSpeed=%d\n", TASK_RUNNING(motor),
           TASK_CONTEXT(motor, MotorCtx).actualSpeed);
    printf("sensorMonitor running=%d faulted=%d\n", TASK_RUNNING(sensorMonitor),
           TASK_FAULTED(sensorMonitor));

    return 0;
}
