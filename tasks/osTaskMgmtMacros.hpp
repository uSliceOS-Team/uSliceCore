/**
 * @file osTaskMgmtMacros.hpp
 * @brief Task lifecycle control macros (START_TASK, STOP_TASK,
 * TASK_RUNNING, TASK_FAULTED, DECLARE_TASK, TASK_CONTEXT) for use from
 * outside a task -- e.g. a Logic task, or any other task reaching this
 * one by name. (A task can also manage itself via `self`, see
 * STOP_SELF() in osTaskMacros.hpp; these macros are for reaching
 * *another* task.)
 *
 * @author uSliceOS Team
 */

#pragma once

#include "osTaskCore.hpp"

#define START_TASK(name)    (OS::Tasks::name.taskStart())
#define STOP_TASK(name)     (OS::Tasks::name.taskStop())
#define TASK_RUNNING(name)  (OS::Tasks::name.isRunning())

/// Just a flag: true if this task's context or state machine flagged a
/// fault since its last start. Nothing stops automatically because of
/// it -- for that, see STOP_SELF() (from the task itself) or STOP_TASK()
/// (from here).
#define TASK_FAULTED(name)  (OS::Tasks::name.isFaulted())

/// Forward-declare a task defined elsewhere (via ADD_TASK /
/// ADD_TASK_AND_START in its own .cpp), so it can be reached by name
/// here. Task itself only stores void* + function pointers, so this
/// does NOT require the task's context type to be visible.
#define DECLARE_TASK(name) \
  namespace OS { namespace Tasks { extern Task name; } }

/// Typed access to a task's context, e.g. to set parameters before
/// START_TASK or read results after it stops. CtxType must match
/// exactly what ADD_TASK/ADD_TASK_AND_START used for this task -- not
/// checked automatically, same trust model as CTX(CtxType) inside the
/// task's own body.
#define TASK_CONTEXT(name, CtxType) \
  (*static_cast<CtxType*>(OS::Tasks::name.getRawContext()))
