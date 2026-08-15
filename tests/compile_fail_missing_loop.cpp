/**
 * @file compile_fail_missing_loop.cpp
 * @brief Must not compile: Task requires a loop handler at compile time.
 */

#include "tasks/Task.hpp"

constinit ::uslice::Task invalidTask{::uslice::Task::Definition{
    .entry = nullptr,
    .loop = nullptr,
    .stop = nullptr,
    .context = nullptr,
    .autostart = false,
}};
