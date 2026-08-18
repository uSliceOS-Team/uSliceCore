/**
 * @file test_contextless_and_registration.cpp
 * @brief Explicit registry traversal and implicit initial case selection.
 */

#include "test_task_fixture.hpp"
#include "test_framework.hpp"
#include "test_scheduler_helpers.hpp"

static int events[8] = {};
static int eventCount = 0;
static int firstCaseRuns = 0;
static int secondCaseRuns = 0;

void Loop_declaredFirst([[maybe_unused]] void* rawCtx_, ::uslice::Task* self) {
    switch (self->currentCase()) {
        case 0:
            firstCaseRuns++;
            events[eventCount++] = 12;
            self->gotoCase(1);
            break;
        case 1:
            secondCaseRuns++;
            break;
    }
}
void Stop_declaredFirst([[maybe_unused]] void* rawCtx_,
                        [[maybe_unused]] ::uslice::Task* self) {}

void Loop_declaredSecond([[maybe_unused]] void* rawCtx_, ::uslice::Task* self) {
    switch (self->currentCase()) {
        case 0:
            events[eventCount++] = 22;
            break;
    }
}
void Stop_declaredSecond([[maybe_unused]] void* rawCtx_,
                         [[maybe_unused]] ::uslice::Task* self) {}

constexpr ::uslice::Task::Program declaredFirstProgram{
    .loop = &Loop_declaredFirst,
    .stop = &Stop_declaredFirst,
    .caseCount = 2,
};
constexpr ::uslice::Task::Program declaredSecondProgram{
    .loop = &Loop_declaredSecond,
    .stop = &Stop_declaredSecond,
    .caseCount = 1,
};

constinit TestEmptyContext declaredFirstContext{};
constinit ::uslice::Task declaredFirst{
    ::uslice::Task::Definition<&declaredFirstProgram>{
        .context = &declaredFirstContext,
        .autostart = true,
    }};
constinit TestEmptyContext declaredSecondContext{};
constinit ::uslice::Task declaredSecond{
    ::uslice::Task::Definition<&declaredSecondProgram>{
        .context = &declaredSecondContext,
        .autostart = true,
    }};
constexpr ::uslice::TaskLink declaredSecondLink{&declaredSecond, nullptr};
constexpr ::uslice::TaskLink declaredFirstLink{&declaredFirst,
                                               &declaredSecondLink};
constinit const ::uslice::TaskRegistry testRegistry{&declaredFirstLink};

int main() {
    RUN_PASSES(1); // first Loop calls, declaration order
    CHECK_EQ(eventCount, 2);
    CHECK_EQ(events[0], 12);
    CHECK_EQ(events[1], 22);
    CHECK_EQ(firstCaseRuns, 1);
    CHECK_EQ(secondCaseRuns, 0);

    RUN_PASSES(1); // selected second state
    CHECK_EQ(firstCaseRuns, 1);
    CHECK_EQ(secondCaseRuns, 1);

    return TEST_SUMMARY("test_contextless_and_registration");
}
