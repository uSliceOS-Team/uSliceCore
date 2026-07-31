/**
 * @file osTime.hpp
 * @brief Lightweight software timer engine for cooperative multitasking in embedded systems.
 * @author uSliceOS Team
 */

#pragma once

#include "osTimeCore.hpp"

namespace OS {

namespace Time {

    class Timer {
    private:
        std::uint32_t startTime; ///< Snapshot of osTick when the timer was started.
        std::uint32_t period;

    public:
        /**
         * @brief Default constructor.
         *
         * @warning Not "inactive" in the sense of isExpired() returning false:
         * with startTime = 0 and period = 0, an unsigned (now - 0) >= 0 is
         * always true, so an un-set Timer reports isExpired() == true on the
         * very first check. Call set() before the first check if that's not
         * what you want.
         */
        explicit Timer(void) noexcept : startTime(0), period(0) {}

        /**
         * @brief Starts the timer as a deadline tracker.
         * @param period_ms Time to wait relative to current moment.
         */
        inline void set(std::uint32_t period_ms) {
            startTime = Core::getSystemTick();
            period = period_ms;
        }

        /**
         * @brief Checks if the configured deadline has passed.
         *
         * Uses unsigned arithmetic logic `(now - start) >= period` to strictly handle
         * counter wraparound.
         *
         * @note One-shot, not a peek: once this returns true it sets period to 0,
         * so every subsequent call also returns true (see the constructor's
         * warning) until set() is called again. Checking the same Timer from
         * more than one place without an intervening set() will misbehave.
         *
         * @return true if deadline is reached or passed, false otherwise.
         */
        [[nodiscard]]
        inline bool isExpired(void) {
            if ((Core::getSystemTick() - startTime) >= period) {
                period = 0;
                return true;
            }

            return false;
        }
    };

    class Clock {
    private:
        std::uint32_t startTime; ///< Snapshot of osTick when the clock was started.

    public:
        /**
         * @brief Default constructor.
         */
        explicit Clock(void) noexcept : startTime(Core::getSystemTick()) {}

        /**
         * @brief Sets the reference point for clock.
         */
        inline void start(void) { startTime = Core::getSystemTick(); }

        /**
         * @brief Alias for getMs().
         * @return Delta time in milliseconds.
         */
        [[nodiscard]]
        uint32_t get(void) const { return getMs(); }

        /**
         * @brief Calculates time elapsed since the last call to start().
         * @return Delta time in milliseconds.
         */
        [[nodiscard]]
        uint32_t getMs(void) const { return (Core::getSystemTick() - startTime); }

        /**
         * @brief Calculates time elapsed since the last call to start().
         * @return Delta time in seconds.
         */
        [[nodiscard]]
        uint32_t getS(void) const { return ((Core::getSystemTick() - startTime) / 1000); }

        /**
         * @brief Calculates time elapsed since the last call to start().
         * @return Delta time in minutes.
         */
        [[nodiscard]]
        uint32_t getM(void) const { return ((Core::getSystemTick() - startTime) / 60000); }

        /**
         * @brief Calculates time elapsed since the last call to start().
         * @return Delta time in hours.
         */
        [[nodiscard]]
        uint32_t getH(void) const { return ((Core::getSystemTick() - startTime) / 3600000); }
    };
} // namespace Time
} // namespace OS

