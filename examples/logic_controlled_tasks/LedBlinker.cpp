/**
 * @file LedBlinker.cpp
 * @brief Example task: blinks a fixed number of times, then stops itself.
 *
 * Demonstrates:
 *  - a context struct with no base class (just a plain struct)
 *  - one loop handler with an ordinary switch over generated cases
 *  - explicit state transitions between switch branches
 *  - self-stop through the Task object when the task decides it is done
 *
 * Its context type follows the generated naming convention and is declared in
 * TaskDefinitions.hpp with the other application context types.
 */

#include "TaskDefinitions.hpp"
#include "generated/Tasks.generated.hpp"
#include "tasks/Task.hpp"
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

void Loop_ledBlinker(void* rawCtx_, ::uslice::Task* self) {
    auto* localTask = static_cast<LedBlinkerContext*>(rawCtx_);
    if (!localTask->initialized) {
        GPIO_Init(GPIO_LED);
        localTask->initialized = true;
    }

    using enum example::tasks::LedBlinkerCase;
    switch (static_cast<example::tasks::LedBlinkerCase>(self->currentCase())) {
        case BLINK:
            std::cout << "[ledBlinker] blink (" << localTask->blinksRemaining
                      << " remaining)\n";
            localTask->blinksRemaining--;
            if (localTask->blinksRemaining <= 0) {
                self->gotoCase(static_cast<::uslice::Task::case_t>(DONE));
            }
            break;
        case DONE:
            std::cout << "[ledBlinker] done, stopping myself\n";
            self->stop();
            break;
    }
}

void Stop_ledBlinker([[maybe_unused]] void* rawCtx_,
                     [[maybe_unused]] ::uslice::Task* self) {
    std::cout << "[ledBlinker] stopped (cleanup ran)\n";
    GPIO_Write(GPIO_LED, 0);
}
