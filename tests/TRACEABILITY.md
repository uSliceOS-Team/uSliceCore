# Test Traceability

This is an informal requirement-to-test mapping for the host-side test suite.
It exists to make two things checkable at a glance: which behaviors are
actually pinned down by a test, and which test file to look at when one of
them regresses.

**What this is not:** a device-level requirements/hazard traceability matrix
as would be expected in an IEC 62304 Software Development Plan. There is no
formal requirements document behind this table: the "Requirement" column is
the observable behavior the library commits to in the README (mainly
[Task Lifecycle Control](../README.md#task-lifecycle-control),
[Safe Task Lifecycle](../README.md#safe-task-lifecycle), and
[Timers](../README.md#timers)), written down here so it's checked, not just
implied.

| # | Requirement (informal) | Verified by | Notes |
|---|---|---|---|
| 1 | An autostarted task's entry handler has run, and it is `RUNNING`, before the first `Loop()` pass | `test_lifecycle_autostart.cpp` | |
| 2 | An autostarted task's loop count advances by exactly one per scheduler pass | `test_lifecycle_autostart.cpp` | |
| 3 | A manually-started task does not run its entry handler or loop body before `START_TASK` is called | `test_lifecycle_manual_start.cpp` | |
| 4 | After `START_TASK`, the task enters `Guard` (running, entry not yet run) before `Entry` runs on the next pass | `test_lifecycle_manual_start.cpp` | matches [Startup: two workflows](../README.md#startup-two-workflows) |
| 5 | `STOP_SELF()` lets the current pass finish (loop count still reflects the pass that called it) before the task stops | `test_self_stop_and_cleanup.cpp` | |
| 6 | A task's cleanup/stop handler runs exactly once, only after the task actually stops | `test_self_stop_and_cleanup.cpp` | |
| 7 | `RAISE_FAULT()` sets the fault flag but does not stop the task or change control flow beyond exiting the current `SWITCH` | `test_fault_flag.cpp` (`naiveFaulter`) | matches [Known Limitations](../README.md#known-limitations) |
| 8 | A task can be made to actually stop itself after a fault by explicit `CASE` design (`GOTO_CASE` + `STOP_SELF()`), which the library does not do automatically | `test_fault_flag.cpp` (`safeFaulter`) | this is the "Correct Pattern" companion to #7 |
| 9 | `TASK_INSTANCE` produces independent context state per instance under a shared task definition | `test_task_instance.cpp` | |
| 10 | A freshly constructed `Timer` reports `isExpired() == true` before `set()` is ever called | `test_timer.cpp` | matches [Known Limitations](../README.md#known-limitations) |
| 11 | `Timer::isExpired()` is one-shot: stays `true` until the next `set()`, not a re-evaluated peek | `test_timer.cpp` | |
| 12 | `Clock::getMs()` reports elapsed time correctly and resets on `restart()` | `test_timer.cpp` | |
| 13 | `START_TASK`/`STOP_TASK` are silent no-ops when called outside the state they expect (e.g. starting an already-running task) | `test_start_stop_noop_windows.cpp` | matches [Macro Reference](../README.md#task-management) |
| 14 | Loop count is preserved across a stop/restart cycle rather than reset | `test_start_stop_noop_windows.cpp` | |
| 15 | A task declared in one translation unit (`DECLARE_TASK`/`TASK_CONTEXT`) is reachable and its context readable from another, without pulling in the defining file's context type | `test_cross_file_main.cpp` / `test_cross_file_task.cpp` | |

Coverage gaps (known, not yet backed by a test): task registration order
across translation units (undefined by design, see Known Limitations),
`CASES(...)` name collisions within a translation unit, and context type
mismatches across `CTX`/`ADD_TASK`/`TASK_CONTEXT`; these are compile-time or
undefined-behavior cases that a host-side runtime test can't meaningfully
pin down; see [Known Limitations](../README.md#known-limitations) instead.
