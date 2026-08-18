/**
 * @file test_self_stop_and_cleanup.cpp
 * @brief Self-stop and external stop both change state
 * immediately, but the cleanup handler runs on the task's *next*
 * scheduled turn, not the same one -- see Task Lifecycle Timing in
 * docs/TECHNICAL_REFERENCE.md. This test exercises the self-stop path; both
 * share the same Task::stop() implementation.
 */

#include "test_task_fixture.hpp"
#include "test_scheduler_helpers.hpp"
#include "test_framework.hpp"

struct SelfStopCtx {
    int loopCount = 0;
    bool stopRan = false;
};

void Loop_selfStopper(void* rawCtx_, ::uslice::Task* self) {
    auto* localTask = static_cast<SelfStopCtx*>(rawCtx_);
    switch (self->currentCase()) {
        case 0:
            localTask->loopCount++;
            if (localTask->loopCount == 3) {
                self->stop();
            }
            break;
    }
}

void Stop_selfStopper(void* rawCtx_, [[maybe_unused]] ::uslice::Task* self) {
    auto* localTask = static_cast<SelfStopCtx*>(rawCtx_);
    localTask->stopRan = true;
}

constexpr ::uslice::Task::Program selfStopperProgram{
    .loop = &Loop_selfStopper,
    .stop = &Stop_selfStopper,
    .caseCount = 1,
};

constinit SelfStopCtx selfStopperContext{};
constinit ::uslice::Task selfStopper{
    ::uslice::Task::Definition<&selfStopperProgram>{
        .context = &selfStopperContext,
        .autostart = true,
    }};
constexpr ::uslice::TaskLink selfStopperLink{&selfStopper, nullptr};
constinit const ::uslice::TaskRegistry testRegistry{&selfStopperLink};

int main() {
    RUN_PASSES(3); // case x3 -- the third call fires self->stop()

    CHECK_EQ(selfStopperContext.loopCount, 3);
    // State changed to End already, but cleanup hasn't run yet: this is
    // the same pass self->stop() was called on.
    CHECK(!selfStopperContext.stopRan);
    // isRunning() is true throughout End, same as Sync -- only
    // false once actually Stopped. Don't mistake this turn's isRunning()
    // for "still fully active."
    CHECK(selfStopper.isRunning());

    RUN_PASSES(1); // next scheduled turn: cleanup runs
    CHECK(selfStopperContext.stopRan);
    CHECK(!selfStopper.isRunning());

    // The case must not have been called again after self->stop() -- the task
    // was in End, not the dispatch state, on every turn after the third.
    CHECK_EQ(selfStopperContext.loopCount, 3);

    selfStopper.stop(); // fully stopped: ignored
    CHECK(selfStopper.isStopped());

    return TEST_SUMMARY("test_self_stop_and_cleanup");
}
