/**
 * @file osTaskCore.hpp
 * @brief Core Task type and the intrusive task list the supercycle
 * scheduler walks each pass.
 *
 * No base class for user context structs: a task's context is just
 * whatever plain struct you define, with no inheritance requirement.
 * Case tracking and the fault flag live in Task itself instead. Each
 * task's body receives both its context (typed, via CTX) and a pointer
 * to its own Task object (`self`), so a task can manage its own
 * lifecycle -- there is no restriction against a task starting,
 * stopping, or inspecting itself or any other task by name.
 *
 * @author uSliceOS Team
 */

#pragma once

#include <cstddef>
#include <cstdint>

enum class TaskState : std::uint8_t {
  STOPPED = 0,
  GUARD,  // manual start requested, waiting for the buffer turn below
  ENTRY,  // runs the task's TASK_ENTRY handler once, then falls into LOOP
  LOOP,
  STOP
};

struct Task {
public:
  using case_t = std::uint32_t;

private:
  inline static Task* _head = nullptr;
  Task* next = nullptr;
  TaskState state = TaskState::STOPPED;
  bool   faulted_ = false;   // kept next to `state`: both are 1 byte, so
                              // grouping them avoids the alignment padding
                              // that would appear if a 4-byte field sat
                              // between them (see Architecture in README).

  case_t currentCase_ = 0;

  void (*entry)(void*, Task*);
  void (*loop)(void*, Task*);
  void (*stop)(void*, Task*);
  void* ctx;

  uint32_t id;
  inline static uint32_t numOfTasks = 0;
  const char* name;

public:
  // entryFunc/loopFunc/stopFunc are always passed in this fixed order by
  // the ADD_TASK*/TASK_INSTANCE macros (osTaskRegMacros.hpp) -- there is
  // no call site where a caller picks this order by hand, so a wrapper
  // type to prevent swapping them would add indirection without removing
  // any real mistake-risk.
  Task(const char* taskName,
       // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
       void (*entryFunc)(void*, Task*),
       void (*loopFunc)(void*, Task*),
       void (*stopFunc)(void*, Task*),
       void* context = nullptr,
       bool autostart = false)
    : next(_head), entry(entryFunc), loop(loopFunc), stop(stopFunc),
      ctx(context), id(numOfTasks++), name(taskName)
  {
    _head = this;

    // Autostart begins directly in ENTRY: its very first turn already
    // runs the entry handler, no buffer turn spent first. There is no
    // external caller to isolate a manual start's timing from -- see
    // taskStart() and GUARD below.
    if (autostart) { state = TaskState::ENTRY; }
  }

  Task* execute(void) {
    switch (state) {
      case TaskState::STOPPED:
        break;
      case TaskState::GUARD:
        // One inert turn, no side effects: the same one-pass buffer a
        // manual start has always had, now separated from the entry
        // handler itself so autostart (which needs no such buffer) can
        // skip straight to ENTRY instead.
        state = TaskState::ENTRY;
        break;
      case TaskState::ENTRY:
        currentCase_ = 0;
        faulted_ = false;
        if (entry != nullptr) { entry(ctx, this); }
        state = TaskState::LOOP;
        break;
      case TaskState::LOOP:
        // No automatic fault handling: a fault is just a flag now. loop()
        // keeps being called every pass regardless of faulted_ unless the
        // task itself (via `self`) or external code (via STOP_TASK) stops
        // it. See raiseFault() below.
        loop(ctx, this);
        break;
      case TaskState::STOP:
        if (stop != nullptr) { stop(ctx, this); }
        state = TaskState::STOPPED;
        break;
    }
    return next;
  }

  /**
   * Lifecycle control. Callable from anywhere: Logic, another task, or
   * the task itself via `self` (see STOP_SELF() in osTaskMacros.hpp).
   * Both are silent no-ops outside the state they expect, not errors:
   * taskStart() only takes effect on a fully stopped task; taskStop()
   * only takes effect once the task has actually reached LOOP.
   */
  void taskStart(void) { if (state == TaskState::STOPPED) { state = TaskState::GUARD; } }
  void taskStop(void)  { if (state == TaskState::LOOP)    { state = TaskState::STOP; } }

  static Task* getHead() { return _head; }

  TaskState getState(void) const { return state; }
  bool isRunning(void) const { return (state != TaskState::STOPPED); }
  bool isStopped(void) const { return (state == TaskState::STOPPED); }

  /// Current CASE and jump target, used by SWITCH/CASE/GOTO_CASE via `self`.
  case_t getCurrentCase(void) const { return currentCase_; }
  void gotoCase(case_t caseNum) { currentCase_ = caseNum; }

  /**
   * A fault is just a flag. Setting it does not stop the task, does not
   * block loop() from running again, and triggers no kernel action.
   * It's purely informational: the task can check isFaulted() on itself
   * and decide what to do (including STOP_SELF()), and/or external code
   * can poll TASK_FAULTED() for logging/alerting. Nothing happens
   * automatically either way.
   */
  void raiseFault(void) { faulted_ = true; }
  bool isFaulted(void) const { return faulted_; }

  /// Raw context access, for external code (e.g. Logic) or another task.
  /// See TASK_CONTEXT in osTaskMgmtMacros.hpp for the typed wrapper.
  void* getRawContext(void) const { return ctx; }
};
