# Host-side tests

Runs on your PC, no microcontroller needed -- catches lifecycle-timing
and macro-plumbing mistakes before you ever flash real hardware. This is
exactly the kind of check that's easy to get subtly wrong by hand (this
library's own `DECLARE_TASK`/`TASK_CONTEXT` split exists *because* an
earlier version of this exact scenario broke silently); running these
takes a few seconds and catches it immediately instead.

## Running

```sh
./run_tests.sh
```

Optionally pass specific C++ and C compilers:
`./run_tests.sh g++-13 gcc-13`. Requires C++17 and C11. Exits `0` only if
every test passed -- wire it into CI as a gate before a build step that targets
real hardware.

The compile-only 32-bit ABI layout check is separate:

```sh
./run_task_size_32.sh
```

It asks Clang to lay out `Task` for three representative targets: ARM
Cortex-M3/EABI, RISC-V RV32IMAC/ILP32, and the RV32E/ILP32E profile used by
CH32V003. It then checks `sizeof(Task) == 36` with `static_assert`. It does not
link or execute target code, so it needs neither QEMU nor target hardware. The
command succeeds only when all three target compilations satisfy the assertion.

Build artifacts land in `.build/`; delete that folder any time, it's
regenerated on the next run.

## Why one binary per scenario, not one binary with many cases

A task registers itself into a single global linked list during C++ static
initialization (see
[Architecture](../docs/TECHNICAL_REFERENCE.md#architecture)), and there is no
way to unregister it. That's fine for firmware, which only ever needs
one set of tasks for the process's entire lifetime -- but it means nothing in
one test file can be reset before the next one runs *if they lived in
the same process*. So each test file here is its own small program: its
own `main()`, its own dedicated tasks, compiled to its own executable.
`run_tests.sh` builds and runs each one and aggregates the results, the
same way a normal test framework would report individual cases from a
single binary.

## What's covered

- `test_lifecycle_autostart.cpp` -- `ADD_TASK_AND_START`: Entry on the
  first scheduler turn, Loop from the turn after.
- `test_lifecycle_manual_start.cpp` -- `ADD_TASK` + a later
  `START_TASK`: the extra Guard buffer turn before Entry.
- `test_self_stop_and_cleanup.cpp` -- `STOP_SELF()` changes state
  immediately, but cleanup (`TASK_STOP`) waits for the task's next
  scheduled turn.
- `test_fault_flag.cpp` -- `RAISE_FAULT()` is inert on its own (a task
  that never transitions away re-raises it every pass, forever) versus
  the documented correct pattern (`STOP_SELF()` in the same `CASE`).
- `test_task_instance.cpp` -- `TASK_INSTANCE` gives two `Task` objects
  independent context storage while sharing one `Entry_`/`Loop_`/`Stop_`
  triple.
- `test_timer.cpp` -- `OS::Time::Timer`'s fresh-instance trap
  (`isExpired() == true` before the first `set()`), latched-expiry behavior,
  and `OS::Time::Clock` elapsed-time tracking. Direct rollover evidence needs
  a controllable tick source in `time/` and is deferred to version 0.2.0.
- `test_restart_after_cleanup.cpp` -- the supported restart path: legal stop,
  cleanup, fully stopped, then Guard, Entry, and Loop.
- `test_cross_task_handover.cpp` -- outgoing cleanup precedes incoming entry
  for both meaningful relative positions in the task list.
- `test_contextless_and_registration.cpp` -- context-less FSM execution, the
  first implicit `CASES` state, and reverse same-translation-unit registration
  order.
- `test_c_api.c` + `test_c_api.cpp` -- C11 header compilation, mixed C/C++
  linkage, and direct verification of the `osTickISR()` bridge.
- `test_cross_file_task.cpp` + `test_cross_file_main.cpp` -- the actual
  `DECLARE_TASK`/`TASK_CONTEXT` cross-file pattern from Task Lifecycle
  Control, built as two translation units the way a real project would
  split a task from its Logic, not simulated within one file.
- `test_task_size_32.cpp` -- compile-time RAM-layout budget: `Task` is 36
  bytes for Cortex-M3/EABI, RV32IMAC/ILP32, and RV32E/ILP32E. This is run by
  `run_task_size_32.sh`, separately from the executable host tests.

## What isn't covered

- Anything hardware-specific: GPIO, real interrupts, real timer
  peripherals. The core contains no target-specific code within its supported
  32-bit scope (see
  [Integration](../docs/TECHNICAL_REFERENCE.md#integration)); these tests
  exercise the scheduler and macro layer only, using a bounded helper that
  mirrors the manager traversal and driving the C-linkage tick bridge instead
  of real hardware. Extracting a bounded pass shared with `osTaskManager()`
  requires a functional change in `tasks/` and is deferred to version 0.2.0.
- Timing/performance (pass duration, worst-case latency). Bounded timing is an
  architectural expectation, not a verified characteristic; see
  [Timing](../docs/TECHNICAL_REFERENCE.md#timing).
