/**
 * @file test_fault_flag.cpp
 * @brief raiseFault() only sets a flag -- it does not stop the task and
 * does not move it to another state. Two tasks here:
 *  - naiveFaulter: raises a fault without ever transitioning away, so it
 *    re-raises every single pass, forever. This is "just a flag, no
 *    action required" working exactly as documented, not a bug -- but
 *    it's the trap called out in Correct and Incorrect Patterns, and
 *    this test pins down that it really does behave this way rather
 *    than relying on a written description of it.
 *  - safeFaulter: raises a fault AND calls stop() in the same
 *    state, the documented correct pattern.
 */

#include "test_task_fixture.hpp"
#include "test_scheduler_helpers.hpp"
#include "test_framework.hpp"

// --- naiveFaulter: the trap ---

struct NaiveCtx {
    int loopCount = 0;
};

void Loop_naiveFaulter(void* rawCtx_, ::uslice::Task* self) {
    auto* localTask = static_cast<NaiveCtx*>(rawCtx_);
    switch (self->currentCase()) {
        case 0:
            localTask->loopCount++;
            self->raiseFault(); // no lifecycle or control-flow effect
            break;
    }
}

void Stop_naiveFaulter([[maybe_unused]] void* rawCtx_,
                       [[maybe_unused]] ::uslice::Task* self) {}

constexpr ::uslice::Task::Program naiveFaulterProgram{
    .loop = &Loop_naiveFaulter,
    .stop = &Stop_naiveFaulter,
    .caseCount = 1,
};

constinit NaiveCtx naiveFaulterContext{};
constinit ::uslice::Task naiveFaulter{
    ::uslice::Task::Definition<&naiveFaulterProgram>{
        .context = &naiveFaulterContext,
        .autostart = true,
    }};

// --- safeFaulter: the documented correct pattern ---

struct SafeCtx {
    int reading = 0;
};

void Loop_safeFaulter(void* rawCtx_, ::uslice::Task* self) {
    auto* localTask = static_cast<SafeCtx*>(rawCtx_);
    switch (self->currentCase()) {
        case 0:
            localTask->reading++;
            self->gotoCase(1);
            break;
        case 1:
            if (localTask->reading >= 2) {
                self->stop();
                self->raiseFault();
                break;
            }
            self->gotoCase(0);
            break;
    }
}

void Stop_safeFaulter([[maybe_unused]] void* rawCtx_,
                      [[maybe_unused]] ::uslice::Task* self) {}

constexpr ::uslice::Task::Program safeFaulterProgram{
    .loop = &Loop_safeFaulter,
    .stop = &Stop_safeFaulter,
    .caseCount = 2,
};

constinit SafeCtx safeFaulterContext{};
constinit ::uslice::Task safeFaulter{
    ::uslice::Task::Definition<&safeFaulterProgram>{
        .context = &safeFaulterContext,
        .autostart = true,
    }};

constexpr ::uslice::TaskLink safeFaulterLink{&safeFaulter, nullptr};
constexpr ::uslice::TaskLink naiveFaulterLink{&naiveFaulter, &safeFaulterLink};
constinit const ::uslice::TaskRegistry testRegistry{&naiveFaulterLink};

int main() {
    // naiveFaulter: every pass raises the fault again, loopCount keeps
    // climbing, and the task never stops on its own.
    RUN_PASSES(5);
    CHECK_EQ(naiveFaulterContext.loopCount, 5);
    CHECK(naiveFaulter.isFaulted());
    CHECK(naiveFaulter.isRunning()); // still running -- nothing stopped it

    // safeFaulter: READ/CHECK alternate one state per turn, so reaching
    // reading==2 and stopping takes a few turns; five is enough headroom.
    CHECK(safeFaulter.isFaulted());
    CHECK(!safeFaulter.isRunning()); // actually stopped, unlike naiveFaulter

    return TEST_SUMMARY("test_fault_flag");
}
