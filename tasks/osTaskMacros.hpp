/**
 * @file osTaskMacros.hpp
 * @brief Task definition and flow-control macros: CASES, CTX, TASK_ENTRY,
 * TASK_LOOP, TASK_STOP, SWITCH/CASE/GOTO_CASE/RAISE_FAULT/SWITCH_END/STOP_SELF.
 *
 * @author uSliceCore Team
 */

#pragma once

#include "osTaskCore.hpp"

// State enumeration and context binding

/**
 * @def CASES(...)
 * @brief Define state identifiers for a task's switch-based state machine.
 *
 * Expands to a plain enum scoped wherever it's invoked (typically once at
 * file scope in the task's .cpp file). If you define more than one
 * CASES(...) block in the *same* translation unit, give each task's
 * states distinct names; they are not namespaced per task.
 */
#define CASES(...) enum : Task::case_t { __VA_ARGS__ }

/**
 * @def CTX(CtxType)
 * @brief Declares `localTask`, a typed pointer to this task's own context,
 * cast from the raw `void*` the framework passes in.
 *
 * Must be the first line inside a TASK_ENTRY(...), TASK_LOOP(...), or
 * TASK_STOP(...) body that needs to read or change context data. CtxType
 * can be any plain struct; there is no inheritance requirement and no
 * compile-time check that it matches what ADD_TASK used for this task --
 * that's on you, the same way it always was inside a single file.
 */
#define CTX(CtxType) CtxType* localTask = static_cast<CtxType*>(rawCtx_)

// Task definition

/**
 * @def TASK_ENTRY(name)
 * @brief Defines a task's entry (run-once-on-start) handler. Required for
 * every task, same as TASK_LOOP and TASK_STOP -- an empty body is fine
 * if there's nothing to initialize.
 *
 * Runs exactly once per start, right before the task's first CASE on
 * that start, with currentCase_ and the fault flag already reset. Same
 * access as TASK_LOOP: rawCtx_ (via CTX) and self. See the two startup
 * workflows in Safe Task Lifecycle -- autostart tasks run this on their
 * very first scheduler turn (Entry); manually started tasks pass through
 * one extra inert turn first (Guard).
 */
#define TASK_ENTRY(name) void Entry_##name(void* rawCtx_, Task* self)

/**
 * @def TASK_LOOP(name)
 * @brief Defines a task's loop (per-pass) handler, required for every
 * task. Receives its context (`rawCtx_`, use CTX to type it) and a
 * pointer to its own Task object (`self`), so the task can manage its
 * own lifecycle if it wants to.
 */
#define TASK_LOOP(name) void Loop_##name(void* rawCtx_, Task* self)

/// Defines a task's stop (cleanup-on-stop) handler. Required for every task.
#define TASK_STOP(name) void Stop_##name(void* rawCtx_, Task* self)

// Flow control
//
// One SWITCH / SWITCH_END pair per function body: both introduce the
// switchEnd_ label used by GOTO_CASE, and C++ labels are
// function-scoped, so a second SWITCH in the same function would collide.

#define SWITCH switch (self->getCurrentCase()) {

#define CASE(caseNum) case (caseNum)

/**
 * @def GOTO_CASE(caseNum)
 * @brief Jump to another case, always exiting the switch immediately.
 * No fallthrough into the next CASE, regardless of whether it's called
 * at the end of a case or from inside an unbraced `if`.
 */
#define GOTO_CASE(caseNum)                                                     \
    do {                                                                       \
        self->gotoCase(caseNum);                                               \
        goto switchEnd_;                                                       \
    } while (0)

/**
 * @def RAISE_FAULT()
 * @brief Flags a fault on this task and exits the switch immediately,
 * same jump semantics as GOTO_CASE.
 *
 * This is just a flag: raising it does not stop the task and triggers
 * no automatic action. Check self->isFaulted() (or TASK_FAULTED(name)
 * from outside) if you want to react to it -- reacting, including
 * stopping, is up to you.
 */
#define RAISE_FAULT()                                                          \
    do {                                                                       \
        self->raiseFault();                                                    \
        goto switchEnd_;                                                       \
    } while (0)

/**
 * @def SWITCH_END
 * @brief Closes the switch opened by SWITCH.
 *
 * Adds a `default` case for when currentCase somehow holds a value with
 * no matching CASE (corrupted state, out-of-range value written by
 * mistake): flags a fault the same way RAISE_FAULT() does. Nothing more
 * happens automatically -- see RAISE_FAULT() above.
 */
#define SWITCH_END                                                             \
    default:                                                                   \
        RAISE_FAULT();                                                         \
        }                                                                      \
    switchEnd_:;

/**
 * @def STOP_SELF()
 * @brief A task stopping itself. Equivalent to STOP_TASK(name) called
 * from outside, but from inside the task's own body via `self` instead
 * of looking itself up by name.
 */
#define STOP_SELF() (self->taskStop())
