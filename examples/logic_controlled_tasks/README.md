# Example: Logic-controlled tasks

A minimal, runnable demonstration of:
- one-time init via `TASK_START` (`LedBlinker.cpp`'s `GPIO_Init`, `Motor.cpp`
  reading the speed Logic configured before start) -- see the two startup
  workflows in the root `README.md` → Safe Task Lifecycle
- a self-contained task that stops itself (`LedBlinker.cpp`, `STOP_SELF()`)
- a task configured from Logic before it starts (`Motor.cpp` + `MotorCtx.hpp`, `TASK_CONTEXT`)
- the fault flag being purely informational (`SensorMonitor.cpp`, `RAISE_FAULT()`)
- a Logic task reaching all three by name without needing to know about
  most of their contexts (`Registrations.hpp`, `DECLARE_TASK`)

This is a **host-side simulation**: it runs as a normal console program
on your machine, not on a microcontroller. `main.cpp` explains why (no
real timer interrupt to drive `osTickISR()`, and `osTaskManager()` never
returns, which doesn't fit a demo that needs to exit and print). On real
hardware you'd wire up the tick interrupt and call `osTaskManager()`
once instead; see the root `README.md` → Integration.

## Build

From the repository root (the folder containing `uSliceCore.h`):

```sh
g++ -std=c++17 -I. \
  examples/logic_controlled_tasks/LedBlinker.cpp \
  examples/logic_controlled_tasks/Motor.cpp \
  examples/logic_controlled_tasks/SensorMonitor.cpp \
  examples/logic_controlled_tasks/Logic.cpp \
  examples/logic_controlled_tasks/main.cpp \
  tasks/osTaskManager.cpp \
  time/osTickISR.cpp \
  -o example

./example
```

## What to look for in the output

- `ledBlinker` is autostarted: its `TASK_START` (`GPIO_Init`) runs on
  pass 0, its first `MAIN` on pass 1 -- two passes total, no wasted turn.
- `motor` is manually started by `logic` on pass 1: its `TASK_START` only
  runs on pass 2 (one buffer pass in between), first `MAIN` on pass 3.
  Same init-then-main shape as `ledBlinker`, just with one extra pass of
  isolation before it, because nothing started it until `logic` decided to.
- `motor`'s `TASK_START` already sees `targetSpeed=42` -- `logic` set it
  via `TASK_CONTEXT` in the very same pass it called `START_TASK`, and
  it's fully visible by the time `motor`'s own turn comes around.
- `ledBlinker` blinks 5 times, then stops **itself** -- no external
  `STOP_TASK` call anywhere for it.
- `sensorMonitor` raises a fault and stops itself in the same `CASE`;
  `logic` later reports the fault as a diagnostic, after the fact --
  it didn't cause the stop, `sensorMonitor` did.
