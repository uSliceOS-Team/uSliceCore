# µSliceCore Technical Reference

![Version](https://img.shields.io/badge/version-0.1.0--rc-lightgrey)
![License](https://img.shields.io/badge/license-Apache%202.0-blue)
![Language](https://img.shields.io/badge/language-C%2B%2B-orange)
![Status](https://img.shields.io/badge/status-active%20development-yellowgreen)
[![CI](https://github.com/uSliceOS-Team/uSliceCore/actions/workflows/ci.yml/badge.svg)](https://github.com/uSliceOS-Team/uSliceCore/actions/workflows/ci.yml)

A lightweight cooperative scheduler for C++17 firmware on supported 32-bit targets, built around a single main loop and finite-state machines. Its architecture is intended to make task timing analyzable; bounded timing has not yet been verified as a project characteristic.

## Table of Contents

- [The Core Idea](#the-core-idea)
- [Quick Start](#quick-start)
- [Design Properties](#design-properties)
- [Task Lifecycle Control](#task-lifecycle-control)
- [Task Lifecycle Timing](#task-lifecycle-timing)
- [Timers](#timers)
- [Architecture](#architecture)
- [Macro Reference](#macro-reference)
- [Correct and Incorrect Patterns](#correct-and-incorrect-patterns)
- [Integration](#integration)
- [Known Limitations](#known-limitations)
- [Safety & Process Notes](#safety--process-notes)
- [Current Status](#current-status)
- [Documentation](#documentation)
- [Contributing](#contributing)
- [License](#license)

---

## The Core Idea

µSliceCore replaces preemptive scheduling with a cooperative main loop. On each iteration, it traverses all registered tasks in the order produced during C++ static initialization and gives each task one scheduler turn. There is no context switching, no separate stack per task, and no scheduler that can interrupt task execution at an arbitrary point. Registration order is defined within one translation unit but is not guaranteed across translation units; see [Known Limitations](#known-limitations).

Most tasks are structured as finite-state machines, although a task that completes in a single short step does not need explicit states. Each FSM state performs a short, non-blocking unit of work and returns. Ideally, that unit is a single operation, but it may be a short sequence of operations chosen by the developer. The design goal is to keep task steps short and maximize the number of useful loop iterations per second without sacrificing clarity or correctness. The actual iteration rate depends on the target, compiler settings, number of registered tasks, and work performed by each task.

This architecture is useful when task execution must remain serialized and lifecycle sequencing must be easy to inspect. It is expected to make the scheduler-induced interval between two turns of the same task analyzable as one complete traversal of the task list. That expectation is not yet a verified timing characteristic of the project. Establishing a real bound requires testing and worst-case execution-time analysis on the actual target, including every task handler, scheduler overhead, and total interrupt interference.

---

## Quick Start

Reproducible implementation and API problems that remain open are tracked in
the living [`known issues`](KNOWN_ISSUES.md) document. This technical reference
is the source of truth for supported public behavior.

**1. Include the headers**

```cpp
#include "uSliceCore.h"
#include "tasks/osTaskCore.hpp"
#include "tasks/osTaskMacros.hpp"
#include "tasks/osTaskRegMacros.hpp"
#include "time/osTime.hpp"
```

`uSliceCore.h`, at the repository root, is a thin umbrella that pulls in `osTaskManager()` (declared in `tasks/osTaskManager.h`) and `osTickISR()` (declared in `time/osTickISR.h`). These are the two calls exposed with C linkage. It is the only header that a plain C file (e.g. a vendor-generated `main.c`) needs, and it is intended for user code only. Internal library files never include it; each `.cpp` includes only the declaration header for the function it implements. Everything else -- `CASES`, `TASK_ENTRY`/`TASK_LOOP`/`TASK_STOP`, `ADD_TASK`, and the rest of the macro API -- is C++-only and lives under `tasks/` and `time/`.

The repository is one folder containing `uSliceCore.h`, `tasks/`, and `time/`. Add that folder as your include directory. From your own code, reach into a subfolder with `tasks/...` or `time/...` (as the snippets on this page do), just as `uSliceCore.h` itself reaches `tasks/osTaskManager.h` and `time/osTickISR.h`. Inside the library, files refer to neighbors in the same folder by bare quoted names instead. For example, `tasks/osTaskMacros.hpp` includes `"osTaskCore.hpp"`, not `"tasks/osTaskCore.hpp"`. This path resolves relative to the including file's directory and requires no additional include-path setup.

**2. Define your task's states**

```cpp
CASES(BLINK);
```

The first listed state is the initial state. `CASES` uses the enum's implicit
dense numbering (`0, 1, 2, ...`); assigning numeric values manually is not
supported in this version. The plain unscoped enum is an intentional part of
the compact macro DSL, so state names must be unique within a translation
unit.

No `SETUP` state is needed here: one-time initialization has its own dedicated handler (`TASK_ENTRY`, step 4), so a task's `CASES` only needs to list its actual running states.

**3. Define your task's context**

A task's data, apart from its current `CASE`, lives in a context object you own. A plain struct is the simplest option, but the framework accepts any type that can be default-initialized by the registration macro. No base class or inheritance is required. Case tracking and the fault flag live in the task's own `Task` object rather than in its context.

```cpp
struct LedBlinkerCtx {
    OS::Time::Timer timer;
};
```

**4. Write the entry handler**

The entry handler runs exactly once after each start, before the task's first `TASK_LOOP` invocation. For an FSM-based task, it therefore also runs before the first `CASE`. It is required for every task, as are the loop and stop handlers in steps 5 and 6; an empty body is fine if there is nothing to initialize. It has the same access as the loop body: the context (`rawCtx_`, typed via `CTX`) and `self`.

```cpp
TASK_ENTRY(ledBlinker) {
    GPIO_Init(GPIO_LED);
}
```

**5. Write the loop handler**

Every task handler receives a pointer to its own `Task` object (`self`). A task registered with a context also receives `rawCtx_`, which `CTX` converts to the declared context type. `self` is what lets a task manage its own lifecycle; see [Task Lifecycle Control](#task-lifecycle-control).

```cpp
TASK_LOOP(ledBlinker) {
    CTX(LedBlinkerCtx);
    SWITCH
        CASE(BLINK):
            if (localTask->timer.isExpired()) {
                localTask->timer.set(100); // ms
                GPIO_Toggle(GPIO_LED);
            }
            break;
    SWITCH_END
}
```

**6. Write the stop handler**

The stop handler is required for every task, for the same reason as the entry handler in step 4.

```cpp
TASK_STOP(ledBlinker) {
    GPIO_Write(GPIO_LED, 0); // turn the LED off during cleanup
}
```

**7. Register the task**

```cpp
ADD_TASK_AND_START(ledBlinker, LedBlinkerCtx);
```

**8. Wire the millisecond tick**

`Timer` and `Clock` need one tick per millisecond to measure time in milliseconds. Producing that tick is unavoidably platform-specific, so it is not part of this repository. Call the tick hook from a hardware timer interrupt configured for a nominal rate of 1 kHz. There are two equivalent ways to call it; use the one that matches the file containing your ISR:

```cpp
// From a C++ file:
void YourHardwareTimerISR(void) {
    OS::Time::Core::onTickISR();
}
```

```c
/* From a plain C file (e.g. a vendor-generated SysTick_Handler.c),
   including only uSliceCore.h: */
void YourHardwareTimerISR(void) {
    osTickISR();
}
```

**9. Start the scheduler**

`osTaskManager()`, declared in `uSliceCore.h`, runs the main scheduling loop forever. It never returns. `main` can be C or C++; only `uSliceCore.h` needs to be included here.

```c
int main(void) {
    osTaskManager();
    // unreachable
}
```

One rule: every task step must be short relative to your system's target-specific, verified latency budget. Do not use blocking calls or busy-wait loops. A long-running step delays every task that is waiting for its next turn.

A complete, runnable multi-task example (self-stopping tasks, a task configured from outside before it starts, and the fault flag in practice) lives in [`examples/logic_controlled_tasks`](../examples/logic_controlled_tasks); see its own README for build instructions.

---

## Design Properties

µSliceCore is a cooperative, single-stack scheduler. Tasks are never interrupted in the middle of a step.

**Consequences of this model:**

- The scheduler structure is expected to make the interval between two turns of the same task analyzable as one full traversal of the task list. No execution-time bound is currently claimed: it must be established on the actual target by measurement and worst-case analysis of handler execution, scheduler overhead, and total interrupt interference.
- There is no context-switching overhead, and no stack is allocated per task.
- Task startup and shutdown turn sequencing is defined and covered by host-side tests (see [Task Lifecycle Timing](#task-lifecycle-timing)); this does not establish wall-clock timing.
- A task's context requires no base class or inheritance, but there is no compile-time verification that a given context type is the "right" one for a given task (see [Known Limitations](#known-limitations)).
- Nothing in the framework restricts who can start, stop, or configure a task. A task can manage its own lifecycle via `self`; any file that includes the right declarations can reach any task by name. Keeping that access disciplined (e.g. "only Logic reaches other tasks") is a convention you choose to follow, not something the library enforces.

**Trade-off:** every task step must be short and non-blocking. This model is not suited to workloads that require long-running computation or blocking I/O within a single step.

---

## Task Lifecycle Control

A task can manage its own lifecycle directly, using `self` (see [Quick Start](#quick-start)):

```cpp
CASE(DONE):
    STOP_SELF();   // equivalent to STOP_TASK(name) called from outside
    break;
```

Nothing stops a task from reaching *other* tasks by name in the same way that any other file does (see below). The framework does not distinguish "a task's own code" from "external code." Whether you keep lifecycle control centralized in one coordinating task (conventionally called **Logic**) or let tasks manage themselves and each other freely depends entirely on how you structure your project.

**Reaching a task by name from another file.** `Task` itself never needs to know a task's context type; it only stores `void*` and function pointers, so declaring a task for use elsewhere doesn't require pulling in that task's context struct:

```cpp
// Registrations.hpp -- included by whichever file(s) need to reach these
// tasks by name (Logic, another task, wherever)
#include "tasks/osTaskMgmtMacros.hpp"

DECLARE_TASK(ledBlinker);
DECLARE_TASK(motor);
```

```cpp
// Logic.cpp
#include "Registrations.hpp"

void checkTask() {
    if (!TASK_RUNNING(ledBlinker)) START_TASK(ledBlinker);
}
```

**Configuring a task's context from outside.** If you need to set parameters on a task before starting it (or read its state after it stops), use `TASK_CONTEXT(name, CtxType)`. Unlike `DECLARE_TASK`, this does need the context type visible at the call site, so only the tasks you actually configure this way need their context struct pulled out into a shared header; everything else can keep its context private to its own `.cpp`, exactly as in [Quick Start](#quick-start):

```cpp
// MotorCtx.hpp -- only exists because something outside motor.cpp
// configures it
#pragma once
struct MotorCtx { int targetSpeed = 0; };
```

```cpp
// Registrations.hpp
#include "MotorCtx.hpp"
DECLARE_TASK(motor);
```

```cpp
// Logic.cpp
TASK_CONTEXT(motor, MotorCtx).targetSpeed = 50;
START_TASK(motor);
```

`CtxType` here must exactly match the type used by `ADD_TASK`/`ADD_TASK_AND_START` for that task. This is not checked automatically: `Task` does not know the concrete type, so there is nothing to check it against. The same trust model applies when writing `CTX(CtxType)` inside a task's own body.

Within the main scheduling loop, tasks run sequentially, so one task may update another task's context without task-to-task synchronization. The update is visible before the target task's next `Loop_` call. See the full example in [`examples/logic_controlled_tasks`](../examples/logic_controlled_tasks).

Only the tick hook is intended to be called directly from an ISR. Task
lifecycle state, fault state, and user context are not synchronized by the
library, so `START_TASK`, `STOP_TASK`, `TASK_RUNNING`, `TASK_FAULTED`, and
`TASK_CONTEXT` have no portable ISR-safety guarantee. Prefer platform-safe
flags written by the ISR and consumed by scheduler tasks. Any direct access
requires target/compiler-specific critical sections and visibility rules
provided entirely by the integrator.

**Fault handling is separate from lifecycle control.** See the next section and [Correct and Incorrect Patterns](#correct-and-incorrect-patterns).

---

## Task Lifecycle Timing

Startup and shutdown follow fixed scheduler-turn rules. A stop request has the same turn sequencing whether it comes from the task itself through `STOP_SELF()` or from other code through `STOP_TASK()`. These rules describe ordering, not verified wall-clock timing.

### Startup: two workflows

Every task runs its `TASK_ENTRY` handler exactly once per start, before its first `TASK_LOOP` invocation, with `currentCase` and the fault flag already reset. For an FSM-based task, this also places `TASK_ENTRY` before the first `CASE`. What happens *before* that handler runs depends on how the task was started:

**1. Autostart (`ADD_TASK_AND_START`): `Entry → Loop`.** The task begins directly in `Entry`. Its first scheduler turn runs `TASK_ENTRY`; its second turn is the first `Loop` call. No turn is spent doing nothing.

**2. Manual/delayed start (`ADD_TASK` + a later `START_TASK`): `Stopped → Guard → Entry → Loop`.** Calling `START_TASK` on a stopped task inserts one inert turn (`Guard`) before `TASK_ENTRY` runs. `TASK_ENTRY` runs after `Guard`, and `Loop` runs for the first time on the following turn.

The `Guard` turn prevents related tasks from overlapping during a transition. For example, two tasks may implement different operating modes of the same device and must never be active at the same time. If a running task receives an accepted stop request and another task is started during the same scheduler turn, the newly started task spends its next turn in `Guard`. This allows the first task to complete `TASK_STOP` before the second task runs `TASK_ENTRY`, regardless of their relative order in the task list. Cleanup may include releasing shared resources or performing any other actions required to finish the previous mode safely. An autostarted task has no runtime transition from another task, so it begins directly in `Entry`.

```
Autostart:      [construct]  --Entry (runs TASK_ENTRY)-->  Loop  --> ...
Manual start:   Stopped  --START_TASK()-->  Guard (inert)  --Entry (runs TASK_ENTRY)-->  Loop  --> ...
```

### Shutdown

**Shutdown happens at the task's next scheduled turn.** When a stop request is accepted, the task enters the `Stop` state. Its `TASK_STOP` handler runs the next time the scheduler reaches that task in its traversal of the list.

It helps to stop thinking in terms of "this pass" vs. "next pass" and instead picture the scheduling loop as an infinite sequence of turns, one per task, repeating forever: `... T1 T2 T3 T1 T2 T3 T1 T2 T3 ...`. In that view, there is only one rule: **cleanup runs at the very next occurrence of that task in the sequence.** Whether that occurrence is labeled "same pass" or "next pass" in a diagram depends only on where you draw the pass boundary relative to the two tasks involved. The scheduler-turn guarantee is the same either way: cleanup occurs no later than one complete traversal of the task list. This is a sequencing guarantee, not a verified wall-clock duration. The `Guard` turn above provides the corresponding sequencing guarantee at the other end of the lifecycle.

```
Sequence:  ... T1  T2  T3 | T1  T2  T3 | T1  T2  T3 ...
Event:              STOP_TASK(T3) or T3 itself calls STOP_SELF()
Result:    T3's cleanup runs at its very next turn, right here ^
           (labeled "same pass" only because T3 hadn't run yet this pass)
```

```
Sequence:  ... T1  T2  T3 | T1  T2  T3 | T1  T2  T3 ...
Event:                  Something calls STOP_TASK(T1)
Result:    T1 already ran this pass, so its next turn is here ^
           (labeled "next pass" only because of where T1 sits)
```

### Full state diagram

```

(autostart)────────────────────────────────────────────────────────────────┐
                                                                           ▼
Stopped -- START_TASK() ──> Guard (inert, manual start only) ──> Entry (runs TASK_ENTRY)
   ▲                                                                       │
   │                                                                       │
   └────── Stop (cleanup) <───────────────────────────────────── Loop  <───┘
```

`TASK_ENTRY`, `TASK_LOOP`, and `TASK_STOP` are all required. A task missing any of the three cannot be registered: a missing `Entry_name`/`Loop_name`/`Stop_name` function is a linker error, not a runtime check. An empty body is fine if a task genuinely has nothing to do at entry or stop.

The host-side test suite covers this turn sequencing (both workflows, cleanup deferral, and the no-op windows below), and you can run it before touching real hardware -- see [`tests/`](../tests).

---

## Timers

The time module consists of a single global tick counter (`OS::Time::Core`) and two small classes built on top of it (`Timer` and `Clock`). The implementation lives in `time/osTimeCore.hpp` and `time/osTime.hpp`. It does not interact with hardware directly; producing the actual tick is your responsibility (see [Integration](#integration)).

### Core: the tick counter

`OS::Time::Core` maintains a wrapping 32-bit tick counter that is incremented by the platform code:

```cpp
std::uint32_t now = OS::Time::Core::getSystemTick();
```

You'll rarely call this directly. `Timer` and `Clock` use it internally, and both assume every tick is exactly one millisecond.

### Timer: latched deadlines

```cpp
OS::Time::Timer t;
t.set(100);             // arm for 100 ms from now
if (t.isExpired()) { }  // true when (now - start) >= 100
```

Two deliberate aspects of this timer model are worth knowing:

- A freshly constructed `Timer` has a zero period and is deliberately treated as expired. Call `set()` before checking it when you need an initial delay.
- Once a timer expires, it remains in the expired state: `isExpired()` continues to return `true` until `set()` is called again. Calling `set()` starts a new deadline.

Every `set(period)` schedules relative to the tick observed at that call.
Repeatedly setting the same period after handling expiry therefore includes
handler latency in the next interval and can accumulate phase drift. `Timer`
is a relative deadline tracker; this version does not provide a phase-stable
periodic timer primitive.

The deadline check uses unsigned subtraction and handles a single rollover of the 32-bit tick counter correctly. Elapsed times spanning a full 2^32-tick cycle cannot be distinguished from shorter elapsed times.

### Clock: elapsed-time stopwatch

```cpp
OS::Time::Clock c;    // starts running immediately
...
uint32_t ms = c.getMs();
uint32_t s  = c.getS();
uint32_t m  = c.getM();
uint32_t h  = c.getH();
c.start();             // reset the reference point
```

`get()` is just an alias for `getMs()`.

Each getter takes its own tick snapshot. `getS()`, `getM()`, and `getH()` return
total elapsed whole units, not display components. Derive multiple display
components from one `getMs()` value when they must represent one coherent
instant. Like `Timer`, `Clock` cannot distinguish an interval spanning a
complete 32-bit tick cycle from a shorter wrapped interval.

### Memory

`Core` holds one static `volatile uint32_t`, not one value per instance. On typical targets where `uint32_t` has 4-byte size and alignment, `Timer` occupies 8 bytes and `Clock` occupies 4 bytes. Verify these sizes with `sizeof` for your target and toolchain.

---

## Architecture

### Intrusive Linked List

Namespace-scope `Task` objects register themselves in a linked list when their C++ dynamic initialization runs before `main()`, prepending themselves to the head. This is automatic registration during C++ static initialization, not compile-time registration. The scheduler holds a pointer to the head and traverses the list during each iteration. The scheduler and time modules perform no dynamic memory allocation. User-provided task handlers remain outside this guarantee.

```
(head) [ Task N ] --> [ Task N-1 ] --> ... --> [ Task 1 ] --> nullptr
```

Because registration prepends, execution order within a single translation unit is the reverse of declaration order. See [Known Limitations](#known-limitations) for what this does and doesn't guarantee across multiple files.

### Task State and Context

Case tracking and the fault flag live in `Task` itself, not in your context struct. `Task` only ever holds an opaque `void*` to your context; it never needs to know the concrete type, which is what lets `DECLARE_TASK` reach a task by name without pulling in that task's context struct (see [Task Lifecycle Control](#task-lifecycle-control)). The cost of this is that there's no compile-time check tying a `Task` to "the right" context type; see [Known Limitations](#known-limitations).

```
┌─────────────────────────────────────┐
│ Task (per task)                     │
│  state, currentCase, faulted        │
│  id, name                           │
│  entry / loop / stop (pointers)     │
│  ctx (void*)  ────────────┐         │
│  next (linked list)       │         │
└───────────────────────────┼─────────┘
                            ▼
              ┌──────────────────────────┐
              │ Your context (per task)  │
              │ whatever fields you add, │
              │ no base class            │
              └──────────────────────────┘
```

One `Entry_`/`Loop_`/`Stop_` triple can be reused across several `Task` instances, each with its own context, via `TASK_INSTANCE`. Useful when the same logic drives several identical peripherals (e.g. one motor-control function, several motors).

### Memory per Task

Not measured on target hardware; derived from struct layout on a 32-bit target (ARM Cortex-M / RISC-V); verify with `sizeof(Task)` if this matters for your budget. `state` and the fault flag are deliberately placed next to each other (both single-byte fields) so they share one alignment slot instead of forcing extra padding around a 4-byte field between them.

```
┌─────────────────────────────────────────┐
│  next pointer         (4 bytes)         │
│  state + faulted      (4 bytes, padded) │
│  currentCase          (4 bytes)         │
│  entry handler        (4 bytes)         │
│  loop handler         (4 bytes)         │
│  stop handler         (4 bytes)         │
│  ctx pointer          (4 bytes)         │
│  id                   (4 bytes)         │
│  name pointer         (4 bytes)         │
├─────────────────────────────────────────┤
│  TOTAL: ~36 bytes                       │
└─────────────────────────────────────────┘
```

This is in addition to your context object, which is a separate static object containing only the data and padding required by its own type, with no framework base-class overhead. An earlier design added about 8 bytes per task for case and fault data inside every context. A bare `Timer` field, for comparison, typically occupies 8 bytes under the assumptions described above.

### Timing

Bounded execution timing is an architectural expectation, not a verified project characteristic. There is no formal timing-validation or benchmark suite yet (see [Documentation](#documentation)), so no pass-duration or worst-case-latency numbers are claimed. Timing depends on the number of registered tasks, what each `CASE` does, scheduler overhead, total interrupt interference, the target MCU, clock speed, and compiler/optimization settings. A bound must be established for the actual integration through target-specific measurement and worst-case execution-time analysis; the necessary instrumentation is not part of the core.

---

## Macro Reference

### Task Definition

*(`tasks/osTaskMacros.hpp`)*

| Macro | Purpose |
|---|---|
| `CASES(...)` | Define implicitly numbered states for one task; the first listed state is initial. Manual numeric values are unsupported |
| `CTX(CtxType)` | First line of a `TASK_ENTRY`/`TASK_LOOP`/`TASK_STOP` body: declares `localTask`, typed |
| `TASK_ENTRY(name)` | Define task entry handler, required, runs once per start before the first `TASK_LOOP` invocation. Same access as `TASK_LOOP`. Empty body if there is nothing to initialize |
| `TASK_LOOP(name)` | Define task loop (per-pass) handler, required. Body has access to `localTask` (via `CTX`) and `self` (this task's own `Task*`) |
| `TASK_STOP(name)` | Define task stop handler, required. Same access as `TASK_LOOP` |

### Flow Control

*(`tasks/osTaskMacros.hpp`)*

| Macro | Purpose |
|---|---|
| `SWITCH` | Begin state machine, reading `self`'s current case |
| `CASE(state)` | Define a state |
| `GOTO_CASE(state)` | Select the state for the next loop invocation and exit the current switch, with no fallthrough |
| `RAISE_FAULT()` | Set the fault flag and immediately exit the current `SWITCH`; it does not stop the task or select another case -- see [Task Lifecycle Control](#task-lifecycle-control) |
| `STOP_SELF()` | Stop this task from inside its own body, using `self` |
| `SWITCH_END` | Close the state machine; unmatched case raises a fault the same way |

### Registration

*(`tasks/osTaskRegMacros.hpp`)*

| Macro | Purpose |
|---|---|
| `ADD_TASK(name, CtxType)` | Register a task with its own context, stopped |
| `ADD_TASK_AND_START(name, CtxType)` | Register a task with its own context and schedule its entry handler on its first turn |
| `ADD_TASK_NO_CTX(name)` | Register a context-less task, stopped |
| `ADD_TASK_NO_CTX_AND_START(name)` | Register a context-less task and schedule its entry handler on its first turn |
| `TASK_INSTANCE(instance, func, CtxType)` | Register another instance of an already-declared task's logic, with its own context |

Context-less tasks cannot use `CTX`, but they may use `SWITCH`, `CASE`, `GOTO_CASE`, and `RAISE_FAULT`. These macros store their state in the task's own `Task` object and do not require a user context. Use a context when a task needs persistent data beyond its current case and fault flag.

### Task Management

*(`tasks/osTaskMgmtMacros.hpp`, for reaching a task from another file -- Logic, another task, wherever)*

| Macro | Purpose |
|---|---|
| `DECLARE_TASK(name)` | Forward-declare a task defined elsewhere, so it can be reached by name here. Never needs the context type |
| `START_TASK(name)` | Start a task; only has an effect if the task is fully stopped |
| `STOP_TASK(name)` | Stop a task; only has an effect if the task is already in `Loop` |
| `TASK_RUNNING(name)` | Check whether the task is in any state other than fully stopped |
| `TASK_FAULTED(name)` | Check whether the fault flag is set. The flag is cleared when the task runs `TASK_ENTRY`; checking it does not trigger any action |
| `TASK_CONTEXT(name, CtxType)` | Typed reference to a task's context, e.g. to set parameters before `START_TASK`. `CtxType` must match what `ADD_TASK` used |

**Check state before calling `START_TASK` / `STOP_TASK`.** Both are silent no-ops outside the state they expect, not errors:

- `START_TASK(name)` only takes effect if the task is fully stopped (`TASK_RUNNING(name)` is `false`). Calling it on a task that's already starting, running, or stopping does nothing.
- `STOP_TASK(name)` only takes effect once the task has actually reached `Loop`. In particular, calling it during the starting window (`Guard` and/or `Entry`, depending on how the task was started -- see [Task Lifecycle Timing](#task-lifecycle-timing)) is a no-op: the task keeps starting and `STOP_TASK` must be called again once it is actually running. `TASK_RUNNING(name)` being `true` is not by itself enough to know that a `STOP_TASK` call will take effect, since it is also `true` throughout that entire starting window.

The same two rules apply to `STOP_SELF()` from inside a task's own body -- it's the same underlying call as `STOP_TASK`, just reached via `self` instead of by name.

---

## Correct and Incorrect Patterns

**Non-blocking step:**
```cpp
CASE(SENSOR_READ):
    value = ADC_Read();
    GOTO_CASE(SENSOR_PROCESSING);
```

**Deferred cleanup, requested externally or by the task itself:**
```cpp
TASK_STOP(motorControl) {
    disable_motor();
    clear_motor_flags();
    reset_position_counters();
}
```

**Blocking delay, never do this:**
```cpp
CASE(WAIT):
    delay_ms(100);  // delays all other scheduler tasks
    GOTO_CASE(NEXT);
```

**Busy-wait loop, never do this:**
```cpp
CASE(WAIT):
    while (!flag) { } // prevents other scheduler tasks from running
    GOTO_CASE(NEXT);
```

**Raising a fault without stopping the task -- easy to trip over:**
```cpp
CASE(CHECK):
    if (sensorOutOfRange()) {
        RAISE_FAULT();   // sets the flag and immediately exits the switch
    }
    GOTO_CASE(READ);
```
`RAISE_FAULT()` does not move the task to another case or stop it, but it does
immediately exit the current `SWITCH`. Consequently, any required action must
happen before it. In the example above, `GOTO_CASE(READ)` is unreachable on the
fault path, so the task lands back in `CHECK` on the next pass and raises the
fault again. To stop and flag the task, use this order:
```cpp
CASE(CHECK):
    if (sensorOutOfRange()) {
        STOP_SELF();     // request the lifecycle change first
        RAISE_FAULT();   // then set the flag and exit the switch
    }
    GOTO_CASE(READ);
```

Reversing those two calls never reaches `STOP_SELF()`. Likewise,
`GOTO_CASE(recovery)` and `RAISE_FAULT()` cannot be sequenced: each exits the
current `SWITCH`, so whichever macro comes second is unreachable. Choose the
required behavior explicitly instead of placing one after the other.

---

## Integration

µSliceCore is a minimal, target-independent core within its supported 32-bit scope. It uses standard C++17 and contains no vendor headers, inline assembly, peripheral drivers, or code tied to a specific MCU. Platform-specific drivers and higher-level modules are outside the scope of this repository and may be provided separately as optional extensions.

The supported targets have a native word size of at least 32 bits and provide
atomic aligned access to the 32-bit tick counter. The core itself contains no
target-specific code within that supported scope. Porting it to a narrower
architecture requires the integrator to select an appropriate counter type and
independently validate atomicity, timer range, rollover behavior, synchronization,
and the resulting memory layout. Such targets are outside the project's
supported scope.

Wiring the core to real hardware requires a small amount of integration work:

1. **The millisecond tick.** Call `OS::Time::Core::onTickISR()` (C++) or `osTickISR()` (plain C, declared in `time/osTickISR.h`, also reachable via the root `uSliceCore.h` umbrella) once per millisecond from an ordinary hardware timer, `SysTick`, or an equivalent source. The accuracy of `Timer` and `Clock` follows the accuracy of this tick source.
2. **The build itself.** The repository is one folder containing `uSliceCore.h`, `tasks/`, and `time/`. Add that single containing folder as your include directory (CMake `target_include_directories`, PlatformIO `lib/`, STM32CubeIDE/MounRiver include paths); this is what lets *your* code reach in with `tasks/osTaskCore.hpp`, `time/osTimeCore.hpp`, and so on. Inside the library itself, files address same-folder neighbors by bare quoted name (`osTaskCore.hpp`, not `tasks/osTaskCore.hpp`), which resolves from the including file's own directory regardless of your include path; only `uSliceCore.h` at the root reaches into `tasks/` and `time/` by folder-qualified name, since those aren't its own neighbors.
3. **Language standard.** The C++ side requires C++17 or later. Only the C-linkage declarations (`tasks/osTaskManager.h`, `time/osTickISR.h`, and the `uSliceCore.h` umbrella over both) need to be reachable from C; everything else under `tasks/` and `time/` is compiled as C++ regardless of what language the rest of your project uses.
4. **Static initialization.** Registration uses namespace-scope C++ dynamic initialization. The platform startup and linker must execute C++ static constructors / `.init_array` before `main`; otherwise the task list remains empty.
5. **Mixed C/C++ projects.** `uSliceCore.h`, `tasks/osTaskManager.h`, and `time/osTickISR.h` all guard their declarations with `#ifdef __cplusplus extern "C" { ... }`, so any of them is safe to include from both a `.c` and a `.cpp` file. Task logic (`.cpp` files using `TASK_ENTRY`/`TASK_LOOP`/`TASK_STOP`/`CASES`/`ADD_TASK`) and a plain-C `main.c` calling only `osTaskManager()`/`osTickISR()` can coexist in the same build; the compiler still needs a C++ toolchain to build the `.cpp` files and link the result.
6. **The example.** [`examples/logic_controlled_tasks`](../examples/logic_controlled_tasks) is a runnable, host-side (not on-target) demonstration of everything in [Task Lifecycle Control](#task-lifecycle-control); it's a good starting point to copy and modify.
7. **Testing before hardware.** [`tests/`](../tests) is a host-side test suite covering lifecycle timing, the fault flag, `TASK_INSTANCE`, `Timer`/`Clock`, and the `DECLARE_TASK`/`TASK_CONTEXT` cross-file pattern -- run `tests/run_tests.sh` on your PC before flashing a change to real hardware. See its own README for what's covered and why it's structured as several small binaries instead of one.

---

## Known Limitations

This summary does not replace the living
[`known-issues document`](KNOWN_ISSUES.md), which records the impact and
planned resolution of each open technical problem.

- **Task registration order across translation units is not guaranteed.** Registration relies on C++ static initialization of `inline` `Task` objects. Within a single translation unit, order is deterministic (reverse of declaration, due to the prepend-to-head list). Across multiple `.cpp` files, the C++ standard does not guarantee relative order. If your tasks do not depend on each other's execution order within a pass, this is a non-issue by design. If you need a specific cross-file order, the tasks are coupled in a way that this scheduler does not arbitrate. A compile-time registration system that removes this dependency entirely is planned (see [Current Status](#current-status)).
- **`CASES(...)` state names are not namespaced per task.** Two `CASES(...)` blocks in the *same* translation unit sharing a name (e.g. both using `SETUP`) will collide. Give states in the same file distinct names, or keep one task per file.
- **`CASES(...)` deliberately uses a plain enum as part of the compact DSL.** State names are unscoped and must be unique within a translation unit. Values must use the implicit dense numbering; manual numeric assignments are unsupported in this version.
- **A task's context has no compile-time type check at all.** Unlike an earlier version of this library, context structs no longer need to derive from anything, and nothing checks whether the `CtxType` values in `CTX(CtxType)`, `ADD_TASK(name, CtxType)`, and `TASK_CONTEXT(name, CtxType)` are all the *same* type for a given task. `Task` stores your context as `void*` and never recovers the concrete type on its own. A mismatch anywhere in that chain is undefined behavior with no diagnostic and can be caught only by you (or a sanitizer) at runtime. Keeping `CtxType` textually consistent across a task's `.cpp`, its registration, and every external use is entirely your responsibility, not the compiler's.
- **`RAISE_FAULT()` does not change control flow beyond exiting the current `SWITCH`.** It does not call `GOTO_CASE`, does not stop the task, and does not prevent the same `CASE` from running (and raising the fault again) on the very next pass. See [Correct and Incorrect Patterns](#correct-and-incorrect-patterns) for the pitfall this creates and how to avoid it.
- **Nothing prevents a task from reaching another task, or itself, in any way `DECLARE_TASK`/`TASK_CONTEXT`/`START_TASK`/`STOP_TASK` allow.** There is no "Logic-only" enforcement; any file that includes the right declarations can call these on any task. Keeping that access disciplined is a project convention, not a compiler-checked one.
- **`START_TASK`, `STOP_TASK`, and `STOP_SELF()` are silent no-ops outside the state they expect, not errors.** See [Task Management](#task-management) in the Macro Reference.

---

## Safety & Process Notes

This section is for anyone evaluating µSliceCore for use in, or alongside,
safety-critical or regulated firmware (e.g. medical devices under IEC 62304).
Read this before assuming more than what's actually here.

Current reproducible defects and unsafe API limitations are tracked in the
living [`known issues`](KNOWN_ISSUES.md) document. Test coverage of the
behavior documented in this reference is mapped in
[`tests/TRACEABILITY.md`](../tests/TRACEABILITY.md).

**What this is not:**
- Not IEC 62304-compliant. Compliance is asserted by a device manufacturer
  for a specific device and its quality management system, not by a
  standalone library. No library can claim this on its own.
- Not formally verified, not third-party audited.
- No SOUP dossier, hazard analysis, or DHF-equivalent documentation exists
  for this project.

**What is deliberately in place:**
- Cooperative main-loop scheduling with no scheduler priorities or task
  preemption. The structure is intended to support bounded pass timing, but no
  such bound is currently claimed. It requires target-specific timing validation
  covering task-handler WCETs, scheduler overhead, and total interrupt
  interference (see [Architecture](#architecture)).
- No dynamic memory allocation anywhere in the core (`tasks/`, `time/`).
- Static analysis: cppcheck (`tools/cppcheck.sh`) and clang-tidy
  (`.clang-tidy`), available locally and enforced by CI on pushes to `main`
  and pull requests targeting `main` (see badge above).
- A MISRA compliance matrix template (`docs/MISRA_COMPLIANCE.md`) is available
  but has not yet been populated. It contains no rule text, per MISRA's license.
- Lightweight requirement-to-test traceability (`tests/TRACEABILITY.md`):
  informal, not a substitute for a device-level hazard/requirements matrix,
  but it shows what behavior each test actually pins down.

**What would still be needed before use in an actual regulated device:**
- A documented software development lifecycle per IEC 62304 (this repo's
  Git history is not that lifecycle by itself)
- Risk analysis per ISO 14971 for the specific integration
- A completed, tool-and-review-verified MISRA compliance report, not the
  in-progress matrix linked above
- Verification records tied to a specific device's DHF

If you're evaluating this for such a use case, treat the above as the
honest starting point, not a checklist that's already been ticked off.

---

## Current Status

**Implemented:**
- Task manager core (cooperative scheduler)
- Dedicated one-time entry handler per task (`TASK_ENTRY`), with distinct autostart vs. manual-start timing (`Entry` vs. `Guard → Entry`)
- Self-managed and externally managed task lifecycle (`STOP_SELF`, `START_TASK`/`STOP_TASK`)
- Cross-file lifecycle access without context types (`DECLARE_TASK`) and explicit typed context access where needed (`TASK_CONTEXT`)
- Fault flag (diagnostic only, no automatic action)
- Task instancing (`TASK_INSTANCE`)
- Millisecond timers (`Timer`, `Clock`)
- Defined and host-tested task handover and lifecycle turn sequencing; wall-clock
  timing remains unverified
- Task registration macros
- Host-side test suite (`tests/`), runnable before targeting real hardware

**Planned:**
- Compile-time task registration (removes dependency on static-init order)
- Some form of compile-time or debug-build context-type checking (see [Known Limitations](#known-limitations))
- A target-specific timing-validation methodology and benchmark suite; bounded
  execution timing will not be claimed until this quality has been tested

---

## Documentation

| Section | Status |
|---|---|
| API Reference | In progress |
| Code examples | 1 runnable example (`examples/logic_controlled_tasks`); more planned |
| Host-side tests | 8 scenarios (`tests/`), see the test README for coverage |
| Benchmarks | Not started, no formal suite yet |
| Bounded timing validation | Not started; currently an architectural expectation, not a verified characteristic |
| Beginner tutorial | Planned |
| Safety-critical coding standard | In progress: see [Safety & Process Notes](#safety--process-notes) |
| Static analysis | cppcheck + clang-tidy, run locally (`tools/`) and enforced by CI for `main` (`.github/workflows/ci.yml`) |
| MISRA compliance | Template only, not yet populated: see `docs/MISRA_COMPLIANCE.md` |
| Requirement/test traceability | Informal, see `tests/TRACEABILITY.md` |

---

## Contributing

The project is at an early stage. Ways to contribute:

- Report bugs via Issues
- Propose improvements via Discussions
- Submit code examples via pull requests
- Report portability issues on C++17 toolchains

---

## License

Apache License 2.0. See [LICENSE](../LICENSE).
