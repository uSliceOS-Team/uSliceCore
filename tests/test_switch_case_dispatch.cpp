/**
 * @file test_switch_case_dispatch.cpp
 * @brief Dispatches one branch of a single Loop switch per scheduler pass and
 * rejects invalid case selections before entering application code.
 */

#include "tasks/Task.hpp"
#include "test_framework.hpp"

struct CaseContext {
    int firstCount = 0;
    int secondCount = 0;
    int stopCount = 0;
};

void Loop_caseTask(void* rawContext, ::uslice::Task* self) {
    auto* context = static_cast<CaseContext*>(rawContext);
    switch (self->currentCase()) {
        case 0:
            context->firstCount++;
            self->gotoCase(1);
            break;
        case 1:
            context->secondCount++;
            self->stop();
            break;
    }
}

void Stop_caseTask(void* rawContext, [[maybe_unused]] ::uslice::Task* self) {
    auto* context = static_cast<CaseContext*>(rawContext);
    context->stopCount++;
}

constexpr ::uslice::Task::Program caseTaskProgram{
    .loop = &Loop_caseTask,
    .stop = &Stop_caseTask,
    .caseCount = 2,
};

constinit CaseContext caseContext{};
constinit ::uslice::Task caseTask{::uslice::Task::Definition<&caseTaskProgram>{
    .context = &caseContext,
    .autostart = true,
}};
constexpr ::uslice::TaskLink caseTaskLink{&caseTask, nullptr};
constinit const ::uslice::TaskRegistry testRegistry{&caseTaskLink};

int main() {
    testRegistry.executePass();
    CHECK_EQ(caseContext.firstCount, 1);
    CHECK_EQ(caseContext.secondCount, 0);
    CHECK_EQ(caseTask.currentCase(), 1U);

    testRegistry.executePass();
    CHECK_EQ(caseContext.secondCount, 1);
    CHECK_EQ(caseContext.stopCount, 0);

    testRegistry.executePass();
    CHECK_EQ(caseContext.stopCount, 1);
    CHECK(caseTask.isStopped());

    CHECK(caseTask.start());
    testRegistry.executePass(); // Sync
    CHECK(!caseTask.isFaulted());
    caseTask.gotoCase(99);
    testRegistry.executePass();
    CHECK(caseTask.isFaulted());
    CHECK(caseTask.state() == ::uslice::TaskState::END);
    CHECK_EQ(caseContext.stopCount, 1);
    testRegistry.executePass();
    CHECK_EQ(caseContext.stopCount, 2);
    CHECK(caseTask.isStopped());

    CHECK(caseTask.start());
    CHECK(!caseTask.isFaulted());
    CHECK_EQ(caseTask.currentCase(), 0U);

    return TEST_SUMMARY("test_switch_case_dispatch");
}
