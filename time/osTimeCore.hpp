/**
 * @file osTimeCore.hpp
 * @brief Lightweight software timer core for cooperative multitasking in embedded systems.
 * @author uSliceCore Team
 */

#pragma once

#include <cstdint>

namespace OS {

namespace Time {

    class Core {
    private:
        inline static volatile std::uint32_t osTick = 0;

    public:
        /**
         * @brief Returns the current system tick (up-counter).
         * @return Current time in system ticks.
         */
        static std::uint32_t getSystemTick() {
            return osTick;
        }

        /**
         * @brief ISR Hook to increment the system tick.
         * @note Call this inside your hardware timer interrupt (e.g., SysTick_Handler),
         * firing at exactly 1 kHz. Every duration in Timer/Clock (osTime.hpp) assumes
         * one tick equals one millisecond; any other rate makes them silently wrong.
         */
        static void onTickISR() { osTick = osTick + 1; }
    };

} // namespace Time
} // namespace OS
