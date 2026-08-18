/**
 * @file test_cross_task_handover.cpp
 * @brief Sync guarantees outgoing cleanup precedes incoming Loop activation for
 * both meaningful relative positions in an explicitly linked task registry.
 */

#include "test_task_fixture.hpp"
#include "test_framework.hpp"
#include "test_scheduler_helpers.hpp"

struct HandoverCtx {
    int cleanupOrder = 0;
    int activationOrder = 0;
};

static int eventOrder = 0;

void Loop_oldAfterController([[maybe_unused]] void* rawCtx_,
                             ::uslice::Task* self) {
    switch (self->currentCase()) {
        case 0:
            break;
    }
}
void Stop_oldAfterController(void* rawCtx_,
                             [[maybe_unused]] ::uslice::Task* self) {
    auto* localTask = static_cast<HandoverCtx*>(rawCtx_);
    localTask->cleanupOrder = ++eventOrder;
}

void Loop_newBeforeController(void* rawCtx_, ::uslice::Task* self) {
    auto* localTask = static_cast<HandoverCtx*>(rawCtx_);
    switch (self->currentCase()) {
        case 0:
            if (localTask->activationOrder == 0) {
                localTask->activationOrder = ++eventOrder;
            }
            break;
    }
}
void Stop_newBeforeController([[maybe_unused]] void* rawCtx_,
                              [[maybe_unused]] ::uslice::Task* self) {}

void Loop_oldBeforeController([[maybe_unused]] void* rawCtx_,
                              ::uslice::Task* self) {
    switch (self->currentCase()) {
        case 0:
            break;
    }
}
void Stop_oldBeforeController(void* rawCtx_,
                              [[maybe_unused]] ::uslice::Task* self) {
    auto* localTask = static_cast<HandoverCtx*>(rawCtx_);
    localTask->cleanupOrder = ++eventOrder;
}

void Loop_newAfterController(void* rawCtx_, ::uslice::Task* self) {
    auto* localTask = static_cast<HandoverCtx*>(rawCtx_);
    switch (self->currentCase()) {
        case 0:
            if (localTask->activationOrder == 0) {
                localTask->activationOrder = ++eventOrder;
            }
            break;
    }
}
void Stop_newAfterController([[maybe_unused]] void* rawCtx_,
                             [[maybe_unused]] ::uslice::Task* self) {}

extern ::uslice::Task oldAfterController;
extern ::uslice::Task newBeforeController;
extern ::uslice::Task oldBeforeController;
extern ::uslice::Task newAfterController;

void Loop_controllerA([[maybe_unused]] void* rawCtx_, ::uslice::Task* self) {
    switch (self->currentCase()) {
        case 0:
            oldAfterController.stop();
            newBeforeController.start();
            self->stop();
            break;
    }
}
void Stop_controllerA([[maybe_unused]] void* rawCtx_,
                      [[maybe_unused]] ::uslice::Task* self) {}

void Loop_controllerB([[maybe_unused]] void* rawCtx_, ::uslice::Task* self) {
    switch (self->currentCase()) {
        case 0:
            oldBeforeController.stop();
            newAfterController.start();
            self->stop();
            break;
    }
}
void Stop_controllerB([[maybe_unused]] void* rawCtx_,
                      [[maybe_unused]] ::uslice::Task* self) {}

constexpr ::uslice::Task::Program oldAfterControllerProgram{
    .loop = &Loop_oldAfterController,
    .stop = &Stop_oldAfterController,
    .caseCount = 1,
};
constexpr ::uslice::Task::Program newBeforeControllerProgram{
    .loop = &Loop_newBeforeController,
    .stop = &Stop_newBeforeController,
    .caseCount = 1,
};
constexpr ::uslice::Task::Program oldBeforeControllerProgram{
    .loop = &Loop_oldBeforeController,
    .stop = &Stop_oldBeforeController,
    .caseCount = 1,
};
constexpr ::uslice::Task::Program newAfterControllerProgram{
    .loop = &Loop_newAfterController,
    .stop = &Stop_newAfterController,
    .caseCount = 1,
};
constexpr ::uslice::Task::Program controllerAProgram{
    .loop = &Loop_controllerA,
    .stop = &Stop_controllerA,
    .caseCount = 1,
};
constexpr ::uslice::Task::Program controllerBProgram{
    .loop = &Loop_controllerB,
    .stop = &Stop_controllerB,
    .caseCount = 1,
};

// Explicit linked traversal order:
// newBefore, controllerA, oldAfter, oldBefore, controllerB, newAfter.
constinit HandoverCtx oldAfterControllerContext{};
constinit ::uslice::Task oldAfterController{
    ::uslice::Task::Definition<&oldAfterControllerProgram>{
        .context = &oldAfterControllerContext,
        .autostart = true,
    }};
constinit HandoverCtx newBeforeControllerContext{};
constinit ::uslice::Task newBeforeController{
    ::uslice::Task::Definition<&newBeforeControllerProgram>{
        .context = &newBeforeControllerContext,
        .autostart = false,
    }};
constinit HandoverCtx oldBeforeControllerContext{};
constinit ::uslice::Task oldBeforeController{
    ::uslice::Task::Definition<&oldBeforeControllerProgram>{
        .context = &oldBeforeControllerContext,
        .autostart = true,
    }};
constinit HandoverCtx newAfterControllerContext{};
constinit ::uslice::Task newAfterController{
    ::uslice::Task::Definition<&newAfterControllerProgram>{
        .context = &newAfterControllerContext,
        .autostart = false,
    }};
constinit HandoverCtx controllerAContext{};
constinit ::uslice::Task controllerA{
    ::uslice::Task::Definition<&controllerAProgram>{
        .context = &controllerAContext,
        .autostart = true,
    }};
constinit HandoverCtx controllerBContext{};
constinit ::uslice::Task controllerB{
    ::uslice::Task::Definition<&controllerBProgram>{
        .context = &controllerBContext,
        .autostart = true,
    }};

constexpr ::uslice::TaskLink newAfterLink{&newAfterController, nullptr};
constexpr ::uslice::TaskLink controllerBLink{&controllerB, &newAfterLink};
constexpr ::uslice::TaskLink oldBeforeLink{&oldBeforeController,
                                           &controllerBLink};
constexpr ::uslice::TaskLink oldAfterLink{&oldAfterController, &oldBeforeLink};
constexpr ::uslice::TaskLink controllerALink{&controllerA, &oldAfterLink};
constexpr ::uslice::TaskLink newBeforeLink{&newBeforeController,
                                           &controllerALink};
constinit const ::uslice::TaskRegistry testRegistry{&newBeforeLink};

int main() {
    RUN_PASSES(1); // both controllers request stop + start
    RUN_PASSES(1); // remaining cleanup/Sync turns
    RUN_PASSES(1); // incoming Loop handlers have now both run

    const HandoverCtx& oldAfter = oldAfterControllerContext;
    const HandoverCtx& newBefore = newBeforeControllerContext;
    const HandoverCtx& oldBefore = oldBeforeControllerContext;
    const HandoverCtx& newAfter = newAfterControllerContext;

    CHECK(oldAfter.cleanupOrder > 0);
    CHECK(newBefore.activationOrder > oldAfter.cleanupOrder);
    CHECK(oldBefore.cleanupOrder > 0);
    CHECK(newAfter.activationOrder > oldBefore.cleanupOrder);

    return TEST_SUMMARY("test_cross_task_handover");
}
