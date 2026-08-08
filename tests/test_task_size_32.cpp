/**
 * @file test_task_size_32.cpp
 * @brief Compile-time Task layout check for supported 32-bit ABIs.
 *
 * This file is compiled, but never linked or executed. The target compiler
 * calculates the layout, and a failed static_assert makes the CI job fail.
 */

#include "tasks/osTaskCore.hpp"

static_assert(sizeof(void*) == 4,
              "this test must use a 32-bit data-pointer ABI");
static_assert(sizeof(void (*)(void*, Task*)) == 4,
              "this test expects 32-bit function pointers");
static_assert(sizeof(Task) == 36,
              "Task layout changed: update the RAM budget intentionally");
