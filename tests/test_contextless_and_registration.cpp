/**
 * @file test_contextless_and_registration.cpp
 * @brief Context-less registration, reverse same-TU traversal, and implicit
 * first CASE selection after entry.
 */

#include "tasks/osTaskCore.hpp"
#include "tasks/osTaskMacros.hpp"
#include "tasks/osTaskMgmtMacros.hpp"
#include "tasks/osTaskRegMacros.hpp"
#include "test_framework.hpp"
#include "test_scheduler_helpers.hpp"

CASES(FIRST_IMPLICIT, SECOND_IMPLICIT);

static int events[8] = {};
static int eventCount = 0;
static int firstCaseRuns = 0;
static int secondCaseRuns = 0;

TASK_ENTRY(declaredFirst) { events[eventCount++] = 11; }
TASK_LOOP(declaredFirst) {
    SWITCH
    CASE(FIRST_IMPLICIT) : firstCaseRuns++;
    events[eventCount++] = 12;
    GOTO_CASE(SECOND_IMPLICIT);
    CASE(SECOND_IMPLICIT) : secondCaseRuns++;
    break;
    SWITCH_END
}
TASK_STOP(declaredFirst) {}

TASK_ENTRY(declaredSecond) { events[eventCount++] = 21; }
TASK_LOOP(declaredSecond) { events[eventCount++] = 22; }
TASK_STOP(declaredSecond) {}

// Prepending makes declaredSecond execute before declaredFirst.
ADD_TASK_NO_CTX_AND_START(declaredFirst);
ADD_TASK_NO_CTX_AND_START(declaredSecond);

int main() {
    CHECK_EQ(static_cast<Task::case_t>(FIRST_IMPLICIT), 0u);
    CHECK_EQ(static_cast<Task::case_t>(SECOND_IMPLICIT), 1u);

    RUN_PASSES(1); // entries, reverse declaration order
    CHECK_EQ(eventCount, 2);
    CHECK_EQ(events[0], 21);
    CHECK_EQ(events[1], 11);

    RUN_PASSES(1); // first loop turn
    CHECK_EQ(events[2], 22);
    CHECK_EQ(events[3], 12);
    CHECK_EQ(firstCaseRuns, 1);
    CHECK_EQ(secondCaseRuns, 0);

    RUN_PASSES(1); // selected second state
    CHECK_EQ(firstCaseRuns, 1);
    CHECK_EQ(secondCaseRuns, 1);

    return TEST_SUMMARY("test_contextless_and_registration");
}
