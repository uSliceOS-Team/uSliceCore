/**
 * @file compile_fail_missing_loop.cpp
 * @brief Must not compile: Task requires a loop handler at compile time.
 */

#include "tasks/Task.hpp"

constexpr ::uslice::Task::Program invalidProgram{
    .loop = nullptr,
    .stop = nullptr,
    .caseCount = 1,
};

constinit ::uslice::Task invalidTask{
    ::uslice::Task::Definition<&invalidProgram>{
        .context = nullptr,
        .autostart = false,
    }};
