/**
 * @file test_timer.cpp
 * @brief OS::Time::Timer edge cases called out in the technical reference's
 * Timers section and Known Limitations. No task scheduler involved here
 * -- Timer only depends on OS::Time::Core, driven directly.
 */

#include "time/osTime.hpp"
#include "test_framework.hpp"

int main() {
    // A freshly constructed Timer reports expired immediately, before
    // set() is ever called -- startTime = period = 0 makes the unsigned
    // deadline math (now - 0) >= 0 always true.
    {
        OS::Time::Timer t;
        CHECK(t.isExpired());
    }

    // After set(period), isExpired() is false until `period` ticks pass,
    // then true from that tick onward (until acknowledged).
    {
        OS::Time::Timer t;
        t.set(5);
        CHECK(!t.isExpired());
        for (int i = 0; i < 4; i++) {
            OS::Time::Core::onTickISR();
        }
        CHECK(!t.isExpired()); // 4 ticks elapsed, deadline is 5
        OS::Time::Core::onTickISR();
        CHECK(t.isExpired()); // 5th tick: deadline reached
    }

    // Latched expiry: once isExpired() returns true, it keeps returning true
    // on every later call, even with no further ticks, until set() establishes
    // a new deadline.
    {
        OS::Time::Timer t;
        t.set(1);
        OS::Time::Core::onTickISR();
        CHECK(t.isExpired());
        CHECK(t.isExpired());  // still true, no ticks needed
        CHECK(t.isExpired());  // still true
        t.set(1000);           // re-arm
        CHECK(!t.isExpired()); // now false again
    }

    // Clock: elapsed time since construction (or the last start()),
    // independent of Timer state.
    {
        OS::Time::Clock c;
        for (int i = 0; i < 10; i++) {
            OS::Time::Core::onTickISR();
        }
        CHECK_EQ(c.getMs(), 10u);
        c.start();
        CHECK_EQ(c.getMs(), 0u);
    }

    // Rollover requires a controllable tick source. Adding one changes code in
    // time/, so direct rollover evidence is deferred to version 0.2.0.

    return TEST_SUMMARY("test_timer");
}
