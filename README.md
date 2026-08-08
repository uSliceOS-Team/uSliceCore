# µSliceCore

![Version](https://img.shields.io/badge/version-0.1.0--rc-lightgrey)
![License](https://img.shields.io/badge/license-Apache%202.0-blue)
![Language](https://img.shields.io/badge/language-C%2B%2B-orange)
![Status](https://img.shields.io/badge/status-active%20development-yellowgreen)
[![CI](https://github.com/uSliceOS-Team/uSliceCore/actions/workflows/ci.yml/badge.svg)](https://github.com/uSliceOS-Team/uSliceCore/actions/workflows/ci.yml)

µSliceCore is a small cooperative scheduler for C++17 firmware on 32-bit
targets. It organizes one main loop into short, orderly tasks. Use it when a
plain superloop has become difficult to maintain, but an RTOS would add features
and costs your application does not need.

## Who it is for

Use µSliceCore when your application can be expressed as short, non-blocking
finite-state-machine steps and you want execution to remain serialized and easy
to inspect. The architecture is intended to make the interval between two turns
of a task analyzable as one complete traversal of the task list. This is a
design expectation, not a verified timing characteristic.

It is not a fit when a task must block, perform long uninterrupted computation,
or meet a latency bound that has not been established by measurement and
worst-case execution-time analysis on the actual target.

## Why not an ordinary superloop?

A plain superloop is excellent while a project is small. As it grows, lifecycle
rules, per-activity state, timers, cleanup, and coordination often become
hand-written conventions spread across `main()`.

µSliceCore keeps the same single-loop and single-stack execution model, but
adds a uniform task structure:

- explicit entry, loop, and stop handlers;
- finite-state-machine and timer primitives;
- defined startup, deferred cleanup, and task-handover sequencing;
- automatic task registration during C++ static initialization and reusable
  task instances;
- host-side tests for scheduler behavior.

In short: it is a structured superloop, not a different concurrency model.

## Why not an RTOS?

An RTOS is the better tool when you need blocking APIs, preemption, priorities,
independent task stacks, or isolation of long-running work. µSliceCore provides
none of those.

In return, µSliceCore has no context switching, no stack per task, and no
dynamic allocation in the scheduler or time core. A task runs only when the
main loop calls it and must return quickly, so execution order and handover are
easy to trace. The design is expected to support bounded timing, but that is not
yet a verified project characteristic. A real bound requires target-specific
testing and analysis of task-handler WCETs, scheduler overhead, and total
interrupt interference.

The core contains no target-specific code and supports 32-bit targets that
provide atomic aligned access to its 32-bit tick counter. Porting it to a
narrower architecture requires the integrator to select an appropriate counter
type and independently validate atomicity, timer range, rollover behavior, and
the resulting memory layout. Such targets are outside the project's supported
scope.

## Minimal shape

```cpp
CASES(BLINK);

struct LedBlinkerCtx {
    OS::Time::Timer timer;
};

TASK_ENTRY(ledBlinker) {}

TASK_LOOP(ledBlinker) {
    CTX(LedBlinkerCtx);
    SWITCH
        CASE(BLINK):
            if (localTask->timer.isExpired()) {
                localTask->timer.set(100);
                GPIO_Toggle(GPIO_LED);
            }
            break;
    SWITCH_END
}

TASK_STOP(ledBlinker) {
    GPIO_Write(GPIO_LED, 0);
}

ADD_TASK_AND_START(ledBlinker, LedBlinkerCtx);
```

Call `osTickISR()` from a nominal 1 kHz hardware timer interrupt and call
`osTaskManager()` from `main()`. Every task step must stay short and
non-blocking.

## Documentation

- [Technical reference](docs/TECHNICAL_REFERENCE.md) — complete API, lifecycle
  timing, timers, architecture, integration, limitations, and safety notes.
- [Runnable example](examples/logic_controlled_tasks) — lifecycle control,
  external configuration, and fault handling.
- [Host-side tests](tests) — supported behavior and test instructions.
- [Known issues](docs/KNOWN_ISSUES.md) — reproducible open implementation and
  API problems.
- [MISRA compliance matrix](docs/MISRA_COMPLIANCE.md) — work-in-progress
  template; not a compliance claim.

The project is in active development. Contributions and portability reports
for C++17 toolchains are welcome.

Apache License 2.0. See [LICENSE](LICENSE).
