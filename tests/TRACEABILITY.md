# Test Traceability

The [technical reference](../docs/TECHNICAL_REFERENCE.md) is the source of truth
for supported public behavior. This file maps that behavior to host-side tests
and records the suite's evidence status.
Open defects and unsafe API limitations are tracked separately in
[`docs/KNOWN_ISSUES.md`](../docs/KNOWN_ISSUES.md).

A test does not turn incidental implementation behavior into a public promise.
Tests that only characterize a known issue are explicitly marked diagnostic.

## Current test mapping

| Supported behavior | Technical reference | Verified by | Status |
|---|---|---|---|
| An autostart task runs `TASK_ENTRY` on its first turn and begins `TASK_LOOP` on its next turn. | [Startup: two workflows](../docs/TECHNICAL_REFERENCE.md#startup-two-workflows) | `test_lifecycle_autostart.cpp` | Release candidate |
| A manual start follows `Stopped -> Guard -> Entry -> Loop`, with an inert Guard turn. | [Startup: two workflows](../docs/TECHNICAL_REFERENCE.md#startup-two-workflows) | `test_lifecycle_manual_start.cpp` | Release candidate |
| A legal stop enters `Stop`; cleanup runs on the task's next scheduled turn, after which it is fully stopped. | [Shutdown](../docs/TECHNICAL_REFERENCE.md#shutdown) | `test_self_stop_and_cleanup.cpp` | Release candidate |
| A supported restart waits for cleanup and full `Stopped`, then follows `Guard -> Entry -> Loop`. | [Task Lifecycle Timing](../docs/TECHNICAL_REFERENCE.md#task-lifecycle-timing) | `test_restart_after_cleanup.cpp` | Release candidate |
| During a cross-task transition, outgoing cleanup precedes incoming entry for either relative task-list order. | [Startup: two workflows](../docs/TECHNICAL_REFERENCE.md#startup-two-workflows), [Shutdown](../docs/TECHNICAL_REFERENCE.md#shutdown) | `test_cross_task_handover.cpp` | Release candidate |
| `TASK_RUNNING` is false only when the task is fully stopped. | [Task Management](../docs/TECHNICAL_REFERENCE.md#task-management) | `test_lifecycle_manual_start.cpp`, `test_self_stop_and_cleanup.cpp` | Release candidate |
| `RAISE_FAULT()` sets the flag, exits the current `SWITCH`, and does not stop the task automatically. | [Flow Control](../docs/TECHNICAL_REFERENCE.md#flow-control), [Correct and Incorrect Patterns](../docs/TECHNICAL_REFERENCE.md#correct-and-incorrect-patterns) | `test_fault_flag.cpp` | Release candidate with diagnostic assertions |
| `TASK_INSTANCE` gives instances independent context storage while sharing handlers. | [Task State and Context](../docs/TECHNICAL_REFERENCE.md#task-state-and-context) | `test_task_instance.cpp` | Release candidate |
| After `Timer::set(period)`, expiry occurs when the elapsed tick count reaches the deadline and remains true until another `set()`. | [Timer: latched deadlines](../docs/TECHNICAL_REFERENCE.md#timer-latched-deadlines) | `test_timer.cpp` | Release candidate with a diagnostic default-state assertion |
| `Clock` reports elapsed milliseconds and `start()` resets its reference point. | [Clock: elapsed-time stopwatch](../docs/TECHNICAL_REFERENCE.md#clock-elapsed-time-stopwatch) | `test_timer.cpp` | Release candidate |
| The public C headers compile as C11, retain C linkage in a mixed C/C++ build, and `osTickISR()` advances the C++ tick core. | [Integration](../docs/TECHNICAL_REFERENCE.md#integration) | `test_c_api.c`, `test_c_api.cpp` | Release candidate |
| Context-less tasks register and run a supported state machine; the first implicit `CASES` value is selected initially. | [Task State and Context](../docs/TECHNICAL_REFERENCE.md#task-state-and-context), [Registration](../docs/TECHNICAL_REFERENCE.md#registration) | `test_contextless_and_registration.cpp` | Release candidate |
| Same-translation-unit registration is traversed in reverse declaration order. | [Architecture](../docs/TECHNICAL_REFERENCE.md#architecture) | `test_contextless_and_registration.cpp` | Release candidate |
| A correctly typed context can be configured outside a task and observed by its later handler turn. | [Task Lifecycle Control](../docs/TECHNICAL_REFERENCE.md#task-lifecycle-control) | `test_cross_file_main.cpp`, `test_cross_file_task.cpp` | GCC-scoped diagnostic because of the documented cross-file ODR issue |
| `Task` occupies 36 bytes with the checked Cortex-M3/EABI, RV32IMAC/ILP32, and RV32E/ILP32E targets. | [Architecture](../docs/TECHNICAL_REFERENCE.md#architecture) | `test_task_size_32.cpp`, `run_task_size_32.sh` | Compile-time ABI/RAM-budget check |

## Diagnostic coverage

The following assertions in otherwise useful tests are diagnostic rather than
release requirements:

- repeated fault raising when the task remains in the same case;
- a default-constructed `Timer` reporting expired before its first `set()`.

## Evidence deferred to 0.2.0

The following gaps cannot be closed without functional changes under `tasks/`
or `time/`, which are outside the allowed scope for the current version:

- direct Timer/Clock verification across a tick rollover requires a
  controllable tick source in `time/`;
- a bounded scheduler driver guaranteed to use the manager's traversal
  requires extracting a single-pass operation from `osTaskManager()` in
  `tasks/`. The current test helper mirrors that traversal, but does not share
  its implementation.

## Documented limitations and invalid uses

The following do not require successful feature tests:

- wrong context types and context access on a context-less task;
- portable cross-translation-unit registration;
- direct ISR task-management or context access;
- stop/start shortcuts and lifecycle cancellation during Guard or Entry;
- an absent Timer disarm operation or drift-free periodic timer;
- manual construction, copying, moving, assignment, or destruction of `Task`.
