/**
 * @file test_cross_task_handover.cpp
 * @brief Guard guarantees outgoing cleanup precedes incoming entry for both
 * meaningful relative positions in the intrusive task list.
 */

#include "tasks/osTaskCore.hpp"
#include "tasks/osTaskMacros.hpp"
#include "tasks/osTaskMgmtMacros.hpp"
#include "tasks/osTaskRegMacros.hpp"
#include "test_framework.hpp"
#include "test_scheduler_helpers.hpp"

CASES(HANDOVER_RUN);

struct HandoverCtx {
    int cleanupOrder = 0;
    int entryOrder = 0;
};

static int eventOrder = 0;

#define DEFINE_OUTGOING(name)                                                 \
    TASK_ENTRY(name) {}                                                       \
    TASK_LOOP(name) {                                                         \
        SWITCH                                                               \
        CASE(HANDOVER_RUN) : break;                                           \
        SWITCH_END                                                           \
    }                                                                         \
    TASK_STOP(name) {                                                        \
        CTX(HandoverCtx);                                                     \
        localTask->cleanupOrder = ++eventOrder;                               \
    }

#define DEFINE_INCOMING(name)                                                 \
    TASK_ENTRY(name) {                                                       \
        CTX(HandoverCtx);                                                     \
        localTask->entryOrder = ++eventOrder;                                 \
    }                                                                         \
    TASK_LOOP(name) {                                                        \
        SWITCH                                                               \
        CASE(HANDOVER_RUN) : break;                                           \
        SWITCH_END                                                           \
    }                                                                         \
    TASK_STOP(name) {}

DEFINE_OUTGOING(oldAfterController)
DEFINE_INCOMING(newBeforeController)
DEFINE_OUTGOING(oldBeforeController)
DEFINE_INCOMING(newAfterController)

DECLARE_TASK(oldAfterController);
DECLARE_TASK(newBeforeController);
DECLARE_TASK(oldBeforeController);
DECLARE_TASK(newAfterController);

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

// Registration prepends. Traversal order becomes:
// newBefore, controllerA, oldAfter, oldBefore, controllerB, newAfter.
ADD_TASK(newAfterController, HandoverCtx);
ADD_TASK_AND_START(controllerB, HandoverCtx);
ADD_TASK_AND_START(oldBeforeController, HandoverCtx);
ADD_TASK_AND_START(oldAfterController, HandoverCtx);
ADD_TASK_AND_START(controllerA, HandoverCtx);
ADD_TASK(newBeforeController, HandoverCtx);

int main() {
    RUN_PASSES(1); // autostart entries
    RUN_PASSES(1); // both controllers request stop + start
    RUN_PASSES(1); // remaining cleanup/guard turns
    RUN_PASSES(1); // incoming entries have now both run

    const HandoverCtx& oldAfter =
        TASK_CONTEXT(oldAfterController, HandoverCtx);
    const HandoverCtx& newBefore =
        TASK_CONTEXT(newBeforeController, HandoverCtx);
    const HandoverCtx& oldBefore =
        TASK_CONTEXT(oldBeforeController, HandoverCtx);
    const HandoverCtx& newAfter =
        TASK_CONTEXT(newAfterController, HandoverCtx);

    CHECK(oldAfter.cleanupOrder > 0);
    CHECK(newBefore.entryOrder > oldAfter.cleanupOrder);
    CHECK(oldBefore.cleanupOrder > 0);
    CHECK(newAfter.entryOrder > oldBefore.cleanupOrder);

    return TEST_SUMMARY("test_cross_task_handover");
}
