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
 * This context is never touched from outside this file, so it stays
 * fully private -- no separate header needed for it (contrast with
 * Motor.cpp, whose context Logic configures externally).
 */

#include "tasks/osTaskMacros.hpp"
#include "tasks/osTaskRegMacros.hpp"
#include <cstdio>

// Host-side stand-ins for real GPIO calls -- see main.cpp for why this
// example doesn't touch real hardware.
#define GPIO_LED 0
static void GPIO_Init(int pin) { printf("  (GPIO_Init(%d))\n", pin); }
static void GPIO_Write(int pin, int on) {
    printf("  (GPIO_Write(%d, %d))\n", pin, on);
}

CASES(BLINK, DONE);

struct LedBlinkerCtx {
    int blinksRemaining = 5;
};

TASK_ENTRY(ledBlinker) {
    printf("[ledBlinker] ENTRY handler: one-time init\n");
    GPIO_Init(GPIO_LED);
}

TASK_LOOP(ledBlinker) {
    CTX(LedBlinkerCtx);
    SWITCH
    CASE(BLINK)
        : printf("[ledBlinker] blink (%d remaining)\n",
                 localTask->blinksRemaining);
    localTask->blinksRemaining--;
    if (localTask->blinksRemaining <= 0) {
        GOTO_CASE(DONE);
    }
    break;

    CASE(DONE) : printf("[ledBlinker] done, stopping myself\n");
    STOP_SELF();
    break;
    SWITCH_END
}

TASK_STOP(ledBlinker) {
    printf("[ledBlinker] stopped (cleanup ran)\n");
    GPIO_Write(GPIO_LED, 0);
}

// Autostart: this task doesn't need Logic to kick it off. Its very first
// scheduler turn already runs TASK_ENTRY above -- no wasted pass before
// initialization, unlike a manually started task (see Motor.cpp).
ADD_TASK_AND_START(ledBlinker, LedBlinkerCtx);
