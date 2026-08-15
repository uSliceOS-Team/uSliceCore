/**
 * @file Macros.hpp
 * @brief Task definition and flow-control macros: CTX, TASK_ENTRY, TASK_LOOP,
 * TASK_STOP, TASK_STATES, TASK_STATE, GOTO_CASE, RAISE_FAULT, and STOP_SELF.
 *
 * @author uSliceCore Team
 */

#pragma once

/**
 * @def CTX(CtxType)
 * @brief Declares `localTask`, a typed pointer to this task's own context,
 * cast from the raw `void*` the framework passes in.
 *
 * Must be the first line inside a TASK_ENTRY(...), TASK_LOOP(...), or
 * TASK_STOP(...) body that needs to read or change context data. CtxType
 * can be any plain struct, optionally cv-qualified (for example,
 * `CTX(const MotorContext)`). Generated registration supplies the same type
 * to the Task object; a future typed handler adapter can remove this cast.
 */
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CTX(CtxType) CtxType* localTask = static_cast<CtxType*>(rawCtx_)

// Task definition

/**
 * @def TASK_ENTRY(name)
 * @brief Defines a task's entry (run-once-on-start) handler. Required for
 * every task, same as TASK_LOOP and TASK_STOP -- an empty body is fine
 * if there's nothing to initialize.
 *
 * Runs exactly once per start, right before the task's first state on
 * that start, with currentCase_ and the fault flag already reset. Same
 * access as TASK_LOOP: rawCtx_ (via CTX) and self. See the two startup
 * workflows in Task Lifecycle Timing -- autostart tasks run this on their
 * very first scheduler turn (Entry); manually started tasks pass through
 * one extra inert turn first (Sync).
 */
#define TASK_ENTRY(name)                                                       \
    void Entry_##name([[maybe_unused]] void* rawCtx_,                          \
                      [[maybe_unused]] ::uslice::Task* self)

/**
 * @def TASK_LOOP(name)
 * @brief Defines a task's loop (per-pass) handler, required for every
 * task. Receives its context (`rawCtx_`, use CTX to type it) and a
 * pointer to its own Task object (`self`), so the task can manage its
 * own lifecycle if it wants to.
 */
#define TASK_LOOP(name)                                                        \
    void Loop_##name([[maybe_unused]] void* rawCtx_,                           \
                     [[maybe_unused]] ::uslice::Task* self)

/// Defines a task's stop (cleanup-on-stop) handler. Required for every task.
#define TASK_STOP(name)                                                        \
    void Stop_##name([[maybe_unused]] void* rawCtx_,                           \
                     [[maybe_unused]] ::uslice::Task* self)

/** Defines the fixed local enum class `State` and exposes its enumerators. */
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define TASK_STATES(...)                                                       \
    enum class State : unsigned char { __VA_ARGS__ };                          \
    using enum State

/** Returns this handler's current value as the local `State` enum class. */
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define TASK_STATE() static_cast<State>(self->currentCase())

/**
 * @def GOTO_CASE(caseNum)
 * @brief Select another case and exit the handler immediately.
 * No fallthrough into the next case, regardless of whether it's called
 * at the end of a case or from inside an unbraced `if`.
 */
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define GOTO_CASE(caseNum)                                                     \
    return self->gotoCase(static_cast<::uslice::Task::case_t>(caseNum))

/**
 * @def RAISE_FAULT()
 * @brief Flags a fault on this task without changing control flow or state.
 *
 * This is just a flag: raising it does not stop the task and triggers
 * no automatic action. Check self->isFaulted() (or TASK_FAULTED(name)
 * from outside) if you want to react to it -- reacting, including
 * stopping, is up to you.
 */
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define RAISE_FAULT() (self->raiseFault())

/**
 * @def STOP_SELF()
 * @brief A task stopping itself. Equivalent to STOP_TASK(name) called
 * from outside, but from inside the task's own body via `self` instead
 * of looking itself up by name.
 */
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define STOP_SELF() (self->stop())
