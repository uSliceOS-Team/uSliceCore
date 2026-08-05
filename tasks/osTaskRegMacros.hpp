/**
 * @file osTaskRegMacros.hpp
 * @brief Task registration macros (ADD_TASK, ADD_TASK_AND_START,
 * ADD_TASK_NO_CTX, ADD_TASK_NO_CTX_AND_START, TASK_INSTANCE).
 *
 * @author uSliceCore Team
 */

#pragma once

#include "osTaskCore.hpp"
#include "osTaskMacros.hpp"

// Registration
//
// A task is: its own static context instance (its data) plus a Task
// object wiring the Entry_/Loop_/Stop_ handlers to it. Two Task
// instances may share the same handlers as long as each has its own
// context instance; see TASK_INSTANCE.

#define TASK_FUNC(name) \
  void Entry_##name(void*, Task*); \
  void Loop_##name(void*, Task*); \
  void Stop_##name(void*, Task*)

/**
 * Internal building block, not meant to be called directly. Constructs a
 * Task bound to a given (already-declared) Entry_/Loop_/Stop_ triple and
 * its own context instance of CtxType (any plain struct, no base class
 * needed).
 */
#define REGISTER_TASK(instance_name, func_name, CtxType, autostart)         \
  namespace OS { namespace Tasks {                                         \
    inline CtxType instance_name##_ctx;                                    \
    inline Task instance_name{#instance_name, Entry_##func_name, Loop_##func_name, \
                               Stop_##func_name, &instance_name##_ctx, autostart}; \
  } }

/// Register a task with its own context, starting stopped.
#define ADD_TASK(name, CtxType)          \
  TASK_FUNC(name);                       \
  REGISTER_TASK(name, name, CtxType, false)

/// Register a task with its own context, starting immediately.
#define ADD_TASK_AND_START(name, CtxType) \
  TASK_FUNC(name);                        \
  REGISTER_TASK(name, name, CtxType, true)

/// Register a context-less task (no state to preserve between calls), starting stopped.
#define ADD_TASK_NO_CTX(name)                                              \
  TASK_FUNC(name);                                                         \
  namespace OS { namespace Tasks {                                         \
    inline Task name{#name, Entry_##name, Loop_##name, Stop_##name};       \
  } }

/// Register a context-less task (no state to preserve between calls), starting immediately.
#define ADD_TASK_NO_CTX_AND_START(name)                                            \
  TASK_FUNC(name);                                                                 \
  namespace OS { namespace Tasks {                                                 \
    inline Task name{#name, Entry_##name, Loop_##name, Stop_##name, nullptr, true};\
  } }

/**
 * Register another instance of an already-declared task's Entry_/Loop_/Stop_
 * functions under a new name, with its own context.
 *
 * Example:
 *   ADD_TASK(motor, MotorCtx);                    // first instance
 *   TASK_INSTANCE(motor_right, motor, MotorCtx);  // second, same logic
 */
#define TASK_INSTANCE(instance_name, func_name, CtxType) \
  REGISTER_TASK(instance_name, func_name, CtxType, false)
