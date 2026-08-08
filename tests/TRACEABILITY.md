# Test Traceability

The README is the source of truth for supported public behavior. This file
maps that behavior to host-side tests and records gaps in the current suite.
Open defects and unsafe API limitations are tracked separately in
[`docs/KNOWN_ISSUES.md`](../docs/KNOWN_ISSUES.md).

A test does not turn incidental implementation behavior into a public promise.
Tests that only characterize a known issue are explicitly marked diagnostic.

## Current test mapping

| Supported behavior | README reference | Verified by | Status |
|---|---|---|---|
| An autostart task runs `TASK_ENTRY` on its first turn and begins `TASK_LOOP` on its next turn. | [Startup: two workflows](../README.md#startup-two-workflows) | `test_lifecycle_autostart.cpp` | Release candidate |
| A manual start follows `Stopped -> Guard -> Entry -> Loop`, with an inert Guard turn. | [Startup: two workflows](../README.md#startup-two-workflows) | `test_lifecycle_manual_start.cpp` | Release candidate |
| A legal stop enters `Stop`; cleanup runs on the task's next scheduled turn, after which it is fully stopped. | [Shutdown](../README.md#shutdown) | `test_self_stop_and_cleanup.cpp` | Release candidate |
| `TASK_RUNNING` is false only when the task is fully stopped. | [Task Management](../README.md#task-management) | `test_lifecycle_manual_start.cpp`, `test_self_stop_and_cleanup.cpp` | Release candidate |
| `RAISE_FAULT()` sets the flag, exits the current `SWITCH`, and does not stop the task automatically. | [Flow Control](../README.md#flow-control), [Correct and Incorrect Patterns](../README.md#correct-and-incorrect-patterns) | `test_fault_flag.cpp` | Release candidate with diagnostic assertions |
| `TASK_INSTANCE` gives instances independent context storage while sharing handlers. | [Task State and Context](../README.md#task-state-and-context) | `test_task_instance.cpp` | Release candidate |
| After `Timer::set(period)`, expiry occurs when the elapsed tick count reaches the deadline and remains true until another `set()`. | [Timer: latched deadlines](../README.md#timer-latched-deadlines) | `test_timer.cpp` | Release candidate with a diagnostic default-state assertion |
| `Clock` reports elapsed milliseconds and `start()` resets its reference point. | [Clock: elapsed-time stopwatch](../README.md#clock-elapsed-time-stopwatch) | `test_timer.cpp` | Release candidate |
| A correctly typed context can be configured outside a task and observed by its later handler turn. | [Task Lifecycle Control](../README.md#task-lifecycle-control) | `test_cross_file_main.cpp`, `test_cross_file_task.cpp` | GCC-scoped diagnostic because of the documented cross-file ODR issue |
| `Task` occupies 36 bytes with the checked Cortex-M3/EABI, RV32IMAC/ILP32, and RV32E/ILP32E targets. | [Architecture](../README.md#architecture) | `test_task_size_32.cpp`, `run_task_size_32.sh` | Compile-time ABI/RAM-budget check |

## Tests requiring revision

`test_start_stop_noop_windows.cpp` mixes a valid stop-wait-start cycle with
calls that are forbidden by the lifecycle protocol, including stop during
Guard and Entry. Its supported portion should be replaced with a test that
performs only:

```text
Loop -> Stop request -> cleanup -> fully Stopped -> Start -> Guard -> Entry -> Loop
```

The current silent rejection of lifecycle commands must not be presented as a
supported application control-flow mechanism.

The following assertions in otherwise useful tests are diagnostic rather than
release requirements:

- repeated fault raising when the task remains in the same case;
- a default-constructed `Timer` reporting expired before its first `set()`.

## Evidence gaps

- cleanup-before-entry cross-task handover for meaningful relative task-list
  positions;
- C11 compilation of the public C headers;
- mixed C/C++ linkage of `osTaskManager()` and `osTickISR()`;
- direct verification of the `osTickISR()` bridge;
- context-less task registration and supported state-machine use;
- same-translation-unit reverse registration order;
- the first implicitly numbered `CASES` state being selected after entry;
- Timer/Clock behavior across a tick rollover, which cannot be exercised
  efficiently without a controllable tick source;
- a bounded scheduler driver that follows the manager's traversal without
  invoking its intentionally infinite outer loop.

## Documented limitations and invalid uses

The following do not require successful feature tests:

- wrong context types and context access on a context-less task;
- portable cross-translation-unit registration;
- direct ISR task-management or context access;
- stop/start shortcuts and lifecycle cancellation during Guard or Entry;
- an absent Timer disarm operation or drift-free periodic timer;
- manual construction, copying, moving, assignment, or destruction of `Task`.
