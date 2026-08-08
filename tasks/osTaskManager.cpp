/**
 * @file osTaskManager.cpp
 * @brief Platform-independent supercycle driver.
 *
 * Traverses the intrusive task list once per pass, calling execute() on
 * every registered task in turn. Contains no platform-specific code:
 * no cycle counters, no vendor headers, nothing tied to a specific MCU.
 *
 * @author uSliceCore Team
 */

#include "osTaskManager.h"
#include "osTaskCore.hpp"

/**
 * @brief Runs the supercycle forever. Never returns.
 *
 * Each pass: walk the list from Task::getHead(), calling execute() on
 * every task. execute() itself returns the next task in the list, so the
 * loop needs no separate pointer to advance it.
 */
extern "C" void osTaskManager(void) {
    static Task* task = nullptr;

    while (true) {
        task = Task::getHead();
        while (task != nullptr) {
            task = task->execute();
        }
    }
}
