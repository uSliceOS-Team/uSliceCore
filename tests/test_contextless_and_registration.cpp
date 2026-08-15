/**
 * @file test_contextless_and_registration.cpp
 * @brief Explicit registry traversal and implicit first CASE selection after
 * entry.
 */

#include "test_task_fixture.hpp"
#include "test_framework.hpp"
#include "test_scheduler_helpers.hpp"

static int events[8] = {};
static int eventCount = 0;
static int firstCaseRuns = 0;
static int secondCaseRuns = 0;

TASK_ENTRY(declaredFirst) { events[eventCount++] = 11; }
TASK_LOOP(declaredFirst) {
    TASK_STATES(FIRST, SECOND);
    switch (TASK_STATE()) {
        case FIRST:
            firstCaseRuns++;
            events[eventCount++] = 12;
            GOTO_CASE(SECOND);
        case SECOND:
            secondCaseRuns++;
            break;
    }
}
TASK_STOP(declaredFirst) {}

TASK_ENTRY(declaredSecond) { events[eventCount++] = 21; }
TASK_LOOP(declaredSecond) { events[eventCount++] = 22; }
TASK_STOP(declaredSecond) {}

TEST_TASK(declaredFirst, TestEmptyContext, true);
TEST_TASK(declaredSecond, TestEmptyContext, true);
constexpr ::uslice::TaskLink declaredSecondLink{&declaredSecond, nullptr};
constexpr ::uslice::TaskLink declaredFirstLink{&declaredFirst,
                                               &declaredSecondLink};
constinit const ::uslice::TaskRegistry testRegistry{&declaredFirstLink};

int main() {
    RUN_PASSES(1); // entries, declaration order
    CHECK_EQ(eventCount, 2);
    CHECK_EQ(events[0], 11);
    CHECK_EQ(events[1], 21);

    RUN_PASSES(1); // first loop turn
    CHECK_EQ(events[2], 12);
    CHECK_EQ(events[3], 22);
    CHECK_EQ(firstCaseRuns, 1);
    CHECK_EQ(secondCaseRuns, 0);

    RUN_PASSES(1); // selected second state
    CHECK_EQ(firstCaseRuns, 1);
    CHECK_EQ(secondCaseRuns, 1);

    return TEST_SUMMARY("test_contextless_and_registration");
}
