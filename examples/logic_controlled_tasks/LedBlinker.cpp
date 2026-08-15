/**
 * @file LedBlinker.cpp
 * @brief Example task: blinks a fixed number of times, then stops itself.
 *
 * Demonstrates:
 *  - a context struct with no base class (just a plain struct)
 *  - TASK_ENTRY: one-time init, no SETUP case needed in the state machine
 *  - GOTO_CASE for normal state transitions
 *  - STOP_SELF(): the task decides on its own that it's done and stops
 *    itself, without any external Logic call.
 *
 * Its context type follows the generated naming convention and is declared in
 * TaskDefinitions.hpp with the other application context types.
 */

#include "TaskDefinitions.hpp"
#include "generated/Tasks.generated.hpp"
#include "tasks/Macros.hpp"
#include <iostream>

// Host-side stand-ins for real GPIO calls -- see main.cpp for why this
// example doesn't touch real hardware.
namespace {
constexpr int GPIO_LED = 0;

void GPIO_Init(int pin) { std::cout << "  (GPIO_Init(" << pin << "))\n"; }
void GPIO_Write(int pin, int on) {
    std::cout << "  (GPIO_Write(" << pin << ", " << on << "))\n";
}
} // namespace

TASK_ENTRY(ledBlinker) {
    std::cout << "[ledBlinker] ENTRY handler: one-time init\n";
    GPIO_Init(GPIO_LED);
}

TASK_LOOP(ledBlinker) {
    TASK_STATES(BLINK, DONE);
    CTX(LedBlinkerContext);
    switch (TASK_STATE()) {
        case BLINK:
            std::cout << "[ledBlinker] blink (" << localTask->blinksRemaining
                      << " remaining)\n";
            localTask->blinksRemaining--;
            if (localTask->blinksRemaining <= 0) {
                GOTO_CASE(DONE);
            }
            break;

        case DONE:
            std::cout << "[ledBlinker] done, stopping myself\n";
            STOP_SELF();
            break;
    }
}

TASK_STOP(ledBlinker) {
    std::cout << "[ledBlinker] stopped (cleanup ran)\n";
    GPIO_Write(GPIO_LED, 0);
}
