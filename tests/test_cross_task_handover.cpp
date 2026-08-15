/**
 * @file test_cross_task_handover.cpp
 * @brief Sync guarantees outgoing cleanup precedes incoming entry for both
 * meaningful relative positions in an explicitly linked task registry.
 */

#include "test_task_fixture.hpp"
#include "test_framework.hpp"
#include "test_scheduler_helpers.hpp"

struct HandoverCtx {
    int cleanupOrder = 0;
    int entryOrder = 0;
};

static int eventOrder = 0;

#define DEFINE_OUTGOING(name)                                                  \
    TASK_ENTRY(name) {}                                                        \
    TASK_LOOP(name) {}                                                         \
    TASK_STOP(name) {                                                          \
        CTX(HandoverCtx);                                                      \
        localTask->cleanupOrder = ++eventOrder;                                \
    }

#define DEFINE_INCOMING(name)                                                  \
    TASK_ENTRY(name) {                                                         \
        CTX(HandoverCtx);                                                      \
        localTask->entryOrder = ++eventOrder;                                  \
    }                                                                          \
    TASK_LOOP(name) {}                                                         \
    TASK_STOP(name) {}

DEFINE_OUTGOING(oldAfterController)
DEFINE_INCOMING(newBeforeController)
DEFINE_OUTGOING(oldBeforeController)
DEFINE_INCOMING(newAfterController)

extern ::uslice::Task oldAfterController;
extern ::uslice::Task newBeforeController;
extern ::uslice::Task oldBeforeController;
extern ::uslice::Task newAfterController;

TASK_ENTRY(controllerA) {}
TASK_LOOP(controllerA) {
    STOP_TASK(oldAfterController);
    START_TASK(newBeforeController);
    STOP_SELF();
}
TASK_STOP(controllerA) {}

TASK_ENTRY(controllerB) {}
TASK_LOOP(controllerB) {
    STOP_TASK(oldBeforeController);
    START_TASK(newAfterController);
    STOP_SELF();
}
TASK_STOP(controllerB) {}

// Explicit linked traversal order:
// newBefore, controllerA, oldAfter, oldBefore, controllerB, newAfter.
TEST_TASK(oldAfterController, HandoverCtx, true);
TEST_TASK(newBeforeController, HandoverCtx, false);
TEST_TASK(oldBeforeController, HandoverCtx, true);
TEST_TASK(newAfterController, HandoverCtx, false);
TEST_TASK(controllerA, HandoverCtx, true);
TEST_TASK(controllerB, HandoverCtx, true);

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
    RUN_PASSES(1); // autostart entries
    RUN_PASSES(1); // both controllers request stop + start
    RUN_PASSES(1); // remaining cleanup/guard turns
    RUN_PASSES(1); // incoming entries have now both run

    const HandoverCtx& oldAfter = TASK_CONTEXT(oldAfterController, HandoverCtx);
    const HandoverCtx& newBefore =
        TASK_CONTEXT(newBeforeController, HandoverCtx);
    const HandoverCtx& oldBefore =
        TASK_CONTEXT(oldBeforeController, HandoverCtx);
    const HandoverCtx& newAfter = TASK_CONTEXT(newAfterController, HandoverCtx);

    CHECK(oldAfter.cleanupOrder > 0);
    CHECK(newBefore.entryOrder > oldAfter.cleanupOrder);
    CHECK(oldBefore.cleanupOrder > 0);
    CHECK(newAfter.entryOrder > oldBefore.cleanupOrder);

    return TEST_SUMMARY("test_cross_task_handover");
}
