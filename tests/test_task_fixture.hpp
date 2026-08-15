/**
 * @file test_task_fixture.hpp
 * @brief Small explicit task definitions used by host-side unit tests.
 */

#pragma once

#include "tasks/Task.hpp"
#include "tasks/Macros.hpp"

struct TestEmptyContext {};

// Test-local shims keep lifecycle tests compact; production code uses the
// generated task objects and their direct Task/context members.
#define TEST_TASK(name, ContextType, startsAutomatically)                      \
    constinit ContextType name##Context{};                                     \
    constinit ::uslice::Task name {                                            \
        ::uslice::Task::Definition<::Loop_##name> {                            \
            .entry = ::Entry_##name, .stop = ::Stop_##name,                    \
            .context = &name##Context, .autostart = startsAutomatically,       \
        }                                                                      \
    }

#define TASK_CONTEXT(name, ContextType) (name##Context)
#define START_TASK(name) (name.start())
#define STOP_TASK(name) (name.stop())
#define TASK_RUNNING(name) (name.isRunning())
#define TASK_FAULTED(name) (name.isFaulted())
