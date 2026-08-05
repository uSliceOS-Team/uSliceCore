# µSliceCore

![Version](https://img.shields.io/badge/version-0.1-lightgrey)
![License](https://img.shields.io/badge/license-Apache%202.0-blue)
![Language](https://img.shields.io/badge/language-C%2B%2B-orange)
![Status](https://img.shields.io/badge/status-active%20development-yellowgreen)
[![CI](https://github.com/uSliceOS-Team/uSliceCore/actions/workflows/ci.yml/badge.svg)](https://github.com/uSliceOS-Team/uSliceCore/actions/workflows/ci.yml)

A lightweight cooperative real-time operating system for microcontrollers based on the superloop architecture and finite state machines. Designed for systems where predictability is not a feature. It is a requirement.

## Table of Contents

- [The Core Idea](#the-core-idea)
- [Quick Start](#quick-start)
- [Design Properties](#design-properties)
- [Task Lifecycle Control](#task-lifecycle-control)
- [Safe Task Lifecycle](#safe-task-lifecycle)
- [Timers](#timers)
- [Architecture](#architecture)
- [Macro Reference](#macro-reference)
- [Correct and Incorrect Patterns](#correct-and-incorrect-patterns)
- [Integration](#integration)
- [Known Limitations](#known-limitations)
- [Safety & Process Notes](#safety--process-notes)
- [Supported Platforms](#supported-platforms)
- [Current Status](#current-status)
- [Documentation](#documentation)
- [Contributing](#contributing)
- [License](#license)

---

## The Core Idea

µSliceCore replaces preemptive scheduling with a deterministic supercycle: an infinite loop that executes one FSM step per registered task per pass. There is no context switching, no stack per task, no scheduler that can interrupt execution at an arbitrary point.

Each task is a finite state machine. Each state does one short, non-blocking operation and returns. The supercycle runs thousands of times per second. The worst-case response time is the sum of all step durations: known, measurable, and bounded.

This is the right architecture when you need to reason formally about timing, when a race condition during task startup or shutdown is unacceptable, or when "it works most of the time" is not good enough.

---

## Quick Start

**1. Include the headers**

```cpp
#include "uSliceCore.h"
#include "tasks/osTaskCore.hpp"
#include "tasks/osTaskMacros.hpp"
#include "tasks/osTaskRegMacros.hpp"
#include "time/osTime.hpp"
```

`uSliceCore.h`, at the repository root, is a thin umbrella that pulls in `osTaskManager()` (declared in `tasks/osTaskManager.h`) and `osTickISR()` (declared in `time/osTickISR.h`): the two calls exposed with C linkage. It's the only header a plain C file (e.g. a vendor-generated `main.c`) needs, and it's meant for user code only: internal library files never include it, each `.cpp` includes only the one declaration header for what it implements. Everything else -- `CASES`, `TASK_ENTRY`/`TASK_LOOP`/`TASK_STOP`, `ADD_TASK`, and the rest of the macro API -- is C++-only and lives under `tasks/` and `time/`.

The repository is one folder containing `uSliceCore.h`, `tasks/`, and `time/`. Add that one containing folder as your include directory: from your own code, reach into a subfolder with `tasks/...` or `time/...` (as the snippets on this page do), the same way `uSliceCore.h` itself reaches `tasks/osTaskManager.h` and `time/osTickISR.h`. Inside the library, files address same-folder neighbors by bare quoted name instead: `tasks/osTaskMacros.hpp` includes `"osTaskCore.hpp"`, not `"tasks/osTaskCore.hpp"`, which resolves from the including file's own directory and needs no include-path setup to work.

**2. Define your task's states**

```cpp
CASES(BLINK);
```

No `SETUP` state needed here: one-time initialization has its own dedicated handler (`TASK_ENTRY`, step 4), so a task's `CASES` only needs to list its actual running states.

**3. Define your task's context**

A task's data (beyond which `CASE` it's in) lives in a small struct you own. It's a plain struct: no base class, no inheritance requirement. Case tracking and the fault flag live in the task's own `Task` object instead, not in your struct, so your struct is exactly your own fields and nothing else.

```cpp
struct LedBlinkerCtx {
    OS::Time::Timer timer;
};
```

**4. Write the entry handler**

Runs exactly once, right before this task's first `CASE` on any given start, required for every task, same as the loop and stop handlers in steps 5 and 6; an empty body is fine if there's nothing to initialize. Same access as the loop body: context (`rawCtx_`, typed via `CTX`) and `self`.

```cpp
TASK_ENTRY(ledBlinker) {
    GPIO_Init(GPIO_LED);
}
```

**5. Write the loop handler**

A task's body gets two things: its context (`rawCtx_`, typed via `CTX`) and a pointer to its own `Task` object (`self`). `self` is what lets a task manage its own lifecycle; see [Task Lifecycle Control](#task-lifecycle-control).

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

Required for every task, for the same reason as step 4.

```cpp
TASK_STOP(ledBlinker) {
    GPIO_Write(GPIO_LED, 0); // guaranteed off on stop
}
```

**7. Register the task**

```cpp
ADD_TASK_AND_START(ledBlinker, LedBlinkerCtx);
```

**8. Wire the millisecond tick**

`Timer`/`Clock` (used above) need a millisecond tick to measure anything. Producing it is unavoidably platform-specific, so it isn't part of this repository: call the tick hook yourself from whichever hardware timer interrupt your MCU fires at exactly 1 kHz. Two equivalent ways to call it, use whichever matches the file your ISR lives in:

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

`osTaskManager()`, declared in `uSliceCore.h`, runs the supercycle forever. It never returns. `main` can be C or C++; only `uSliceCore.h` needs including here.

```c
int main(void) {
    osTaskManager();
    // unreachable
}
```

One rule: every `CASE` must complete in hundreds of nanoseconds to a couple of microseconds. No blocking calls, no busy-wait loops. Violating this delays every other task in the system by the same amount.

A complete, runnable multi-task example (self-stopping tasks, a task configured from outside before it starts, and the fault flag in practice) lives in [`examples/logic_controlled_tasks`](examples/logic_controlled_tasks); see its own README for build instructions.

---

## Design Properties

µSliceCore is a cooperative, single-stack scheduler. Tasks are never interrupted mid-step.

**Consequences of this model:**

- Worst-case response time for any task equals the sum of all step durations across all registered tasks. This value is bounded and measurable.
- There is no context switching overhead and no stack allocated per task.
- Task startup and shutdown sequencing is deterministic (see [Safe Task Lifecycle](#safe-task-lifecycle)).
- A task's context is a plain struct: no inheritance, no compile-time verification that a given struct is the "right" one for a given task (see [Known Limitations](#known-limitations)).
- Nothing in the framework restricts who can start, stop, or configure a task. A task can manage its own lifecycle via `self`; any file that includes the right declarations can reach any task by name. Keeping that access disciplined (e.g. "only Logic reaches other tasks") is a convention you choose to follow, not something the library enforces.

**Trade-off:** every task step must be short and non-blocking. This model is not suited for workloads that require long-running computation or blocking I/O within a single step.

---

## Task Lifecycle Control

A task can manage its own lifecycle directly, using `self` (see [Quick Start](#quick-start)):

```cpp
CASE(DONE):
    STOP_SELF();   // equivalent to STOP_TASK(name) called from outside
    break;
```

Nothing stops a task from reaching *other* tasks by name either, the same way any other file does (see below); the framework doesn't distinguish "a task's own code" from "external code." Whether you keep lifecycle control centralized in one coordinating task (conventionally called **Logic**) or let tasks manage themselves and each other freely is entirely up to how you structure your own project.

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

`CtxType` here must match exactly what `ADD_TASK`/`ADD_TASK_AND_START` used for that task. This isn't checked automatically: `Task` doesn't know the concrete type, so there's nothing to check against; the same trust model as writing `CTX(CtxType)` correctly inside a task's own body.

Cooperative scheduling makes writing into another task's context safe without any synchronization: nothing ever runs "at the same time" as anything else, only strictly one after another within a pass, so a write is always fully visible before the target task's next `Loop_` call. See the full example in [`examples/logic_controlled_tasks`](examples/logic_controlled_tasks).

**Fault handling is separate from lifecycle control.** See the next section and [Correct and Incorrect Patterns](#correct-and-incorrect-patterns).

---

## Safe Task Lifecycle

Startup and shutdown timing follow fixed rules, regardless of whether start/stop was requested by the task itself (`STOP_SELF()`) or from outside (`START_TASK`/`STOP_TASK`).

### Startup: two workflows

Every task runs its `TASK_ENTRY` handler exactly once per start, before its first `CASE`, with `currentCase` and the fault flag already reset. What happens *before* that handler runs depends on how the task was started:

**1. Autostart (`ADD_TASK_AND_START`): `Entry → Loop`.** The task begins directly in `Entry`. Its very first scheduler turn runs `TASK_ENTRY`; its second turn is the first real `Loop` call. No turn is spent doing nothing.

**2. Manual/delayed start (`ADD_TASK` + a later `START_TASK`): `Stopped → Guard → Entry → Loop`.** Calling `START_TASK` on a stopped task inserts one inert turn (`Guard`) before `TASK_ENTRY` runs, the same one-turn buffer manual starts have always had, now separated from initialization instead of being the initialization. Only after `Guard` does `TASK_ENTRY` run, and only on the turn after *that* does `Loop` run for the first time.

The `Guard` turn exists only for manual starts because there's an external caller whose timing needs isolating from the task's own; an autostarted task has no such caller (nothing "calls" it into existence except the program starting), so there's nothing to guard against.

```
Autostart:      [construct]  --Entry (runs TASK_ENTRY)-->  Loop  --> ...
Manual start:   Stopped  --START_TASK()-->  Guard (inert)  --Entry (runs TASK_ENTRY)-->  Loop  --> ...
```

### Shutdown

**Shutdown happens at the task's next scheduled turn.** A stop request just changes the task's internal state; the actual cleanup (`TASK_STOP` handler) runs the next time the scheduler reaches that task in its traversal of the list.

It helps to stop thinking in terms of "this pass" vs. "next pass" and instead picture the supercycle unrolled into an infinite sequence of turns, one per task, repeating forever: `... T1 T2 T3 T1 T2 T3 T1 T2 T3 ...`. In that view there's only one rule: **cleanup runs at the very next occurrence of that task in the sequence.** Whether that occurrence gets labeled "same pass" or "next pass" in a diagram depends only on where you draw the pass boundary relative to the two tasks involved. It's the same guarantee either way, always resolving within one full supercycle pass, which is the same bound the rest of this scheduler is built on. This is exactly the same guarantee the `Guard` turn above is built on too, just applied at the other end of the lifecycle.

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

There's a host-side test suite covering exactly this timing (both workflows, cleanup deferral, the no-op windows below) that you can run before ever touching real hardware -- see [`tests/`](tests).

---

## Timers

Two independent pieces, in `time/osTimeCore.hpp` and `time/osTime.hpp`: a single global tick counter (`OS::Time::Core`), and two small classes built on top of it (`Timer`, `Clock`). Neither touches hardware directly; producing the actual tick is your job (see [Integration](#integration)).

### Core: the tick counter

A single monotonic, wraparound-safe counter, incremented by whatever platform code calls it:

```cpp
std::uint32_t now = OS::Time::Core::getSystemTick();
```

You'll rarely call this directly. `Timer` and `Clock` use it internally, and both assume every tick is exactly one millisecond.

### Timer: one-shot deadlines

```cpp
OS::Time::Timer t;
t.set(100);             // arm for 100 ms from now
if (t.isExpired()) { }  // true once (now - start) >= 100
```

Two things worth knowing before relying on it:

- A freshly constructed `Timer` has never had `set()` called, so `startTime = 0, period = 0`. Unsigned arithmetic makes `(now - 0) >= 0` always true, so `isExpired()` reports `true` on the very first check, immediately, before you ever armed it. Call `set()` first if that's not what you want.
- `isExpired()` is one-shot, not a peek: once it returns `true`, it resets `period` to 0 internally, so every later call also returns `true` until you call `set()` again. Checking the same `Timer` from more than one place without an intervening `set()` will misbehave.

The deadline check uses unsigned wraparound-safe subtraction, so it stays correct across the tick counter's roughly 49.7-day rollover (2^32 ms).

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

### Memory

`Core` holds one `volatile uint32_t`, static, not per-instance. `Timer` is 8 bytes (two `uint32_t`). `Clock` is 4 bytes (one `uint32_t`).

---

## Architecture

### Intrusive Linked List

Tasks register themselves into a linked list at static-init time, prepending to the head. The scheduler holds a pointer to the head and traverses the list each supercycle pass. No dynamic allocation at any point.

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

This is on top of your context struct, which is a separate static object, and now exactly the size of your own fields, with no base-class tax (previously ~8 bytes per task for case+fault, duplicated in every context). A bare `Timer` field, for comparison, is 8 bytes.

### Timing

There is no formal benchmark suite yet (see [Documentation](#documentation)), so no pass-duration numbers are published here. In practice, timing depends on the number of registered tasks, what each `CASE` does, the target MCU, clock speed, and compiler/optimization settings; a number measured on one setup would not transfer to another. Measuring this yourself requires a platform-specific cycle counter or a logic analyzer; that instrumentation isn't part of this platform-independent core.

---

## Macro Reference

### Task Definition

*(`tasks/osTaskMacros.hpp`)*

| Macro | Purpose |
|---|---|
| `CASES(...)` | Define state enumerations for one task |
| `CTX(CtxType)` | First line of a `TASK_ENTRY`/`TASK_LOOP`/`TASK_STOP` body: declares `localTask`, typed |
| `TASK_ENTRY(name)` | Define task entry handler, required, runs once per start before the first `CASE`. Same access as `TASK_LOOP`. Empty body if there's nothing to initialize |
| `TASK_LOOP(name)` | Define task loop (per-pass) handler, required. Body has access to `localTask` (via `CTX`) and `self` (this task's own `Task*`) |
| `TASK_STOP(name)` | Define task stop handler, required. Same access as `TASK_LOOP` |

### Flow Control

*(`tasks/osTaskMacros.hpp`)*

| Macro | Purpose |
|---|---|
| `SWITCH` | Begin state machine, reading `self`'s current case |
| `CASE(state)` | Define a state |
| `GOTO_CASE(state)` | Jump to another state, no fallthrough |
| `RAISE_FAULT()` | Flag a fault deliberately from any CASE, no fallthrough. Just a flag -- see [Task Lifecycle Control](#task-lifecycle-control) |
| `STOP_SELF()` | Stop this task from inside its own body, using `self` |
| `SWITCH_END` | Close the state machine; unmatched case raises a fault the same way |

### Registration

*(`tasks/osTaskRegMacros.hpp`)*

| Macro | Purpose |
|---|---|
| `ADD_TASK(name, CtxType)` | Register a task with its own context, stopped |
| `ADD_TASK_AND_START(name, CtxType)` | Register a task with its own context, running |
| `ADD_TASK_NO_CTX(name)` | Register a context-less task, stopped |
| `ADD_TASK_NO_CTX_AND_START(name)` | Register a context-less task, running |
| `TASK_INSTANCE(instance, func, CtxType)` | Register another instance of an already-declared task's logic, with its own context |

### Task Management

*(`tasks/osTaskMgmtMacros.hpp`, for reaching a task from another file -- Logic, another task, wherever)*

| Macro | Purpose |
|---|---|
| `DECLARE_TASK(name)` | Forward-declare a task defined elsewhere, so it can be reached by name here. Never needs the context type |
| `START_TASK(name)` | Start a task; only has an effect if the task is fully stopped |
| `STOP_TASK(name)` | Stop a task; only has an effect if the task is already in `Loop` |
| `TASK_RUNNING(name)` | Check if task is running |
| `TASK_FAULTED(name)` | Check if the task's context ever raised a fault since its last start (diagnostics only, does not trigger anything) |
| `TASK_CONTEXT(name, CtxType)` | Typed reference to a task's context, e.g. to set parameters before `START_TASK`. `CtxType` must match what `ADD_TASK` used |

**Check state before calling `START_TASK` / `STOP_TASK`.** Both are silent no-ops outside the state they expect, not errors:

- `START_TASK(name)` only takes effect if the task is fully stopped (`TASK_RUNNING(name)` is `false`). Calling it on a task that's already starting, running, or stopping does nothing.
- `STOP_TASK(name)` only takes effect once the task has actually reached `Loop`. In particular, calling it during the starting window (`Guard` and/or `Entry`, depending on how the task was started -- see [Safe Task Lifecycle](#safe-task-lifecycle)) is a no-op: the task keeps starting and `STOP_TASK` must be called again once it's actually running. `TASK_RUNNING(name)` being `true` is not by itself enough to know a `STOP_TASK` call will take effect, since it's also `true` throughout that entire starting window.

The same two rules apply to `STOP_SELF()` from inside a task's own body -- it's the same underlying call as `STOP_TASK`, just reached via `self` instead of by name.

---

## Correct and Incorrect Patterns

**Non-blocking step:**
```cpp
CASE(SENSOR_READ):
    value = ADC_Read(); // ~100 ns
    GOTO_CASE(SENSOR_PROCESSING);
```

**Safe shutdown, external or self-initiated -- same guarantee either way:**
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
    delay_ms(100);  // blocks ALL tasks for 100 ms
    GOTO_CASE(NEXT);
```

**Busy-wait loop, never do this:**
```cpp
CASE(WAIT):
    while (!flag) { } // blocks ALL tasks indefinitely
    GOTO_CASE(NEXT);
```

**Raising a fault without deciding what happens next -- easy to trip over:**
```cpp
CASE(CHECK):
    if (sensorOutOfRange()) {
        RAISE_FAULT();   // just a flag; jumps out of the switch, nothing else
    }
    GOTO_CASE(READ);
```
`RAISE_FAULT()` does not move the task to another case and does not stop it. If the branch that raises it doesn't also call `GOTO_CASE` or `STOP_SELF()`, the task lands right back in `CHECK` next pass and raises the fault again, every pass, forever -- "just a flag, no action required" working exactly as designed, but rarely what you actually want. Decide explicitly:
```cpp
CASE(CHECK):
    if (sensorOutOfRange()) {
        STOP_SELF();     // or GOTO_CASE(some recovery state)
        RAISE_FAULT();   // order between the two doesn't matter here
    }
    GOTO_CASE(READ);
```

---

## Integration

This repository is platform-independent by design: no vendor headers, no inline assembly, nothing tied to a specific MCU. Wiring it to real hardware means writing two small pieces of glue yourself; neither ships here.

1. **The millisecond tick.** Call `OS::Time::Core::onTickISR()` (C++) or `osTickISR()` (plain C, declared in `time/osTickISR.h`, also reachable via the root `uSliceCore.h` umbrella) from a hardware timer interrupt firing at exactly 1 kHz (an ordinary timer, `SysTick`, whatever your platform calls it). See [Timers](#timers) for why the rate has to be exact, not approximate.
2. **The build itself.** The repository is one folder containing `uSliceCore.h`, `tasks/`, and `time/`. Add that single containing folder as your include directory (CMake `target_include_directories`, PlatformIO `lib/`, STM32CubeIDE/MounRiver include paths); this is what lets *your* code reach in with `tasks/osTaskCore.hpp`, `time/osTimeCore.hpp`, and so on. Inside the library itself, files address same-folder neighbors by bare quoted name (`osTaskCore.hpp`, not `tasks/osTaskCore.hpp`), which resolves from the including file's own directory regardless of your include path; only `uSliceCore.h` at the root reaches into `tasks/` and `time/` by folder-qualified name, since those aren't its own neighbors.
3. **Language standard.** The C++ side requires C++17 or later. Only the C-linkage declarations (`tasks/osTaskManager.h`, `time/osTickISR.h`, and the `uSliceCore.h` umbrella over both) need to be reachable from C; everything else under `tasks/` and `time/` is compiled as C++ regardless of what language the rest of your project uses.
4. **Mixed C/C++ projects.** `uSliceCore.h`, `tasks/osTaskManager.h`, and `time/osTickISR.h` all guard their declarations with `#ifdef __cplusplus extern "C" { ... }`, so any of them is safe to include from both a `.c` and a `.cpp` file. Task logic (`.cpp` files using `TASK_ENTRY`/`TASK_LOOP`/`TASK_STOP`/`CASES`/`ADD_TASK`) and a plain-C `main.c` calling only `osTaskManager()`/`osTickISR()` can coexist in the same build; the compiler still needs a C++ toolchain to build the `.cpp` files and link the result.
5. **The example.** [`examples/logic_controlled_tasks`](examples/logic_controlled_tasks) is a runnable, host-side (not on-target) demonstration of everything in [Task Lifecycle Control](#task-lifecycle-control); it's a good starting point to copy and modify.
6. **Testing before hardware.** [`tests/`](tests) is a host-side test suite covering lifecycle timing, the fault flag, `TASK_INSTANCE`, `Timer`/`Clock`, and the `DECLARE_TASK`/`TASK_CONTEXT` cross-file pattern -- run `tests/run_tests.sh` on your PC before flashing a change to real hardware. See its own README for what's covered and why it's structured as several small binaries instead of one.

---

## Known Limitations

- **Task registration order across translation units is not guaranteed.** Registration relies on C++ static initialization of `inline` `Task` objects. Within a single translation unit, order is deterministic (reverse of declaration, due to the prepend-to-head list). Across multiple `.cpp` files, the C++ standard does not guarantee relative order. If your tasks must not depend on each other's execution order within a pass to behave correctly, this is a non-issue by design; if you think you need a specific cross-file order, that's a sign the tasks are coupled in a way this scheduler doesn't arbitrate; a compile-time registration system that removes this dependency entirely is planned (see [Current Status](#current-status)).
- **`CASES(...)` state names are not namespaced per task.** Two `CASES(...)` blocks in the *same* translation unit sharing a name (e.g. both using `SETUP`) will collide. Give states in the same file distinct names, or keep one task per file.
- **`CASES(...)` values are a plain `enum`, not `enum class`.** They implicitly convert to `Task::case_t` (a `uint32_t`) with no scoping and no compile-time protection against mixing values from different tasks.
- **A task's context has no compile-time type check at all.** Unlike an earlier version of this library, context structs no longer need to derive from anything, and there is nothing checking that the `CtxType` you write in `CTX(CtxType)`, `ADD_TASK(name, CtxType)`, and `TASK_CONTEXT(name, CtxType)` are all the *same* type for a given task. `Task` stores your context as `void*` and never recovers the concrete type on its own; a mismatch anywhere in that chain is undefined behavior with no diagnostic, caught only by you (or a sanitizer) at runtime. Keeping `CtxType` textually consistent across a task's `.cpp`, its registration, and anywhere it's reached externally is entirely your discipline now, not the compiler's.
- **`RAISE_FAULT()` does not change control flow beyond exiting the current `SWITCH`.** It does not call `GOTO_CASE`, does not stop the task, and does not prevent the same `CASE` from running (and raising the fault again) on the very next pass. See [Correct and Incorrect Patterns](#correct-and-incorrect-patterns) for the pitfall this creates and how to avoid it.
- **Nothing prevents a task from reaching another task, or itself, in any way `DECLARE_TASK`/`TASK_CONTEXT`/`START_TASK`/`STOP_TASK` allow.** There is no "Logic-only" enforcement; any file that includes the right declarations can call these on any task. Keeping that access disciplined is a project convention, not a compiler-checked one.
- **A freshly constructed `Timer` reports `isExpired() == true` on its very first check**, before you ever call `set()` on it, because of how the unsigned deadline math resolves at `startTime = period = 0`. This is not the same as "inactive."
- **`Timer::isExpired()` is a one-shot check, not a peek.** Once it returns `true`, it will keep returning `true` on every subsequent call until `set()` is called again. Checking the same `Timer` from more than one place without an intervening `set()` will misbehave.
- **Context-less tasks (`ADD_TASK_NO_CTX`, `ADD_TASK_NO_CTX_AND_START`) cannot use the state-machine macros.** `CTX`, `SWITCH`, `CASE`, `GOTO_CASE`, and `RAISE_FAULT` all rely on `self` (for state machine flow) and `localTask` (for data) being available; a context-less task still has `self`, but has no `CTX(CtxType)` line and thus no `localTask`, so any macro relying specifically on context data doesn't apply. Use `ADD_TASK_NO_CTX` only for a handler that's a single flat function body with no internal state; anything that needs states or per-task data needs a real context via `ADD_TASK` / `ADD_TASK_AND_START`.
- **`START_TASK`, `STOP_TASK`, and `STOP_SELF()` are silent no-ops outside the state they expect, not errors.** See the note under [Task Management](#macro-reference) in the Macro Reference.

---

## Safety & Process Notes

This section is for anyone evaluating µSliceCore for use in, or alongside,
safety-critical or regulated firmware (e.g. medical devices under IEC 62304).
Read this before assuming more than what's actually here.

**What this is not:**
- Not IEC 62304-compliant. Compliance is asserted by a device manufacturer
  for a specific device and its quality management system, not by a
  standalone library. No library can claim this on its own.
- Not formally verified, not third-party audited.
- No SOUP dossier, hazard analysis, or DHF-equivalent documentation exists
  for this project.

**What is deliberately in place:**
- Deterministic supercycle scheduling: no preemption, no priority inversion,
  bounded worst-case pass time (see [Architecture](#architecture)).
- No dynamic memory allocation anywhere in the core (`tasks/`, `time/`).
- Static analysis: cppcheck (`tools/cppcheck.sh`) and clang-tidy
  (`.clang-tidy`), enforced on every push/PR via CI (see badge above) as well
  as locally.
- A MISRA compliance matrix (`docs/MISRA_COMPLIANCE.md`) tracking rule IDs
  and status only; no rule text, per MISRA's license.
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

## Supported Platforms

| Platform | Status | Vendors |
|---|---|---|
| STM32/GD32/APM32 | Priority | ST, GigaDevice, Geehy |
| CH32 | Priority | WCH |
| ESP32 | Not planned | Espressif |

ESP32 is not planned: ESP-IDF is built on FreeRTOS as its execution model, and proper µSliceCore support would require a full alternative to ESP-IDF. That is outside the current scope of this project.

---

## Current Status

**Ready for use:**
- Task manager core (supercycle scheduler)
- Dedicated one-time entry handler per task (`TASK_ENTRY`), with distinct autostart vs. manual-start timing (`Entry` vs. `Guard → Entry`)
- Self-managed and externally managed task lifecycle (`STOP_SELF`, `START_TASK`/`STOP_TASK`)
- Cross-file task access without pulling in context types (`DECLARE_TASK`, `TASK_CONTEXT`)
- Fault flag (diagnostic only, no automatic action)
- Task instancing (`TASK_INSTANCE`)
- Millisecond timers (`Timer`, `Clock`)
- Safe task lifecycle
- Task registration macros
- Host-side test suite (`tests/`), runnable before targeting real hardware

**Planned:**
- Board examples (STM32 and others)
- Compile-time task registration (removes dependency on static-init order)
- Some form of compile-time or debug-build context-type checking (see [Known Limitations](#known-limitations))
- FIFO & LIFO buffer
- UART & SPI & I2C libraries
- SD card driver with lightweight filesystem for logging
- UART-based profiling utility
- DMA library
- IDE extensions

---

## Documentation

| Section | Status |
|---|---|
| API Reference | In progress |
| Code examples | 1 runnable example (`examples/logic_controlled_tasks`); more planned |
| Host-side tests | 8 scenarios (`tests/`), see its own README for coverage |
| Benchmarks | Not started, no formal suite yet |
| Beginner tutorial | Planned |
| Safety-critical coding standard | In progress: see [Safety & Process Notes](#safety--process-notes) |
| Static analysis | cppcheck + clang-tidy, run locally (`tools/`) and enforced in CI (`.github/workflows/ci.yml`) |
| MISRA compliance | Template only, not yet populated: see `docs/MISRA_COMPLIANCE.md` |
| Requirement/test traceability | Informal, see `tests/TRACEABILITY.md` |

---

## Contributing

The project is in early stage. Ways to contribute:
- Report bugs via Issues
- Propose improvements via Discussions
- Submit code examples via Pull Request
- Add platform support

---

## License

Apache License 2.0, see [LICENSE](LICENSE)
