# Known Issues

This living document tracks reproducible implementation defects and unsafe API
limitations that remain relevant to the current project. Intentional design
properties and supported behavior belong in the README, not here. Resolved
issues should be removed from this list when their fixes, tests, and
documentation land; release history belongs in the changelog.

## Registration and object lifetime

### Cross-file registration is not portable standard C++

Registration defines namespace-scope `inline` objects in a `.cpp`, while
`DECLARE_TASK` provides only an `extern` declaration in another translation
unit that odr-uses the object. GCC accepts the current example and test, but an
inline definition is required to be reachable in every translation unit where
the object is odr-used. The documented pattern is therefore not a portable
standard-conforming C++17 interface.

In addition, relative dynamic-initialization order across translation units is
not guaranteed. The current same-translation-unit order remains deterministic.

**Impact:** treat the cross-file pattern as GCC-specific current evidence and
do not make behavior depend on cross-translation-unit registration order.

**Planned resolution:** redesign registration around a standard-conforming
definition model and explicit deterministic ordering.

### Task can be copied, moved, or manually constructed

`Task` is publicly constructible, copyable, movable, and assignable even though
its links and lifetime belong to an intrusive namespace-scope registry.
`execute()` and `getHead()` are also public implementation/test hooks.

**Impact:** application code must create tasks only through registration
macros. Manually constructing, copying, moving, assigning, or dynamically
destroying a `Task` can violate registry assumptions. Application code must not
call `execute()` or `getHead()`.

**Planned resolution:** delete unsafe special members and restrict construction
and internal hooks.

## State machines and lifecycle

### RAISE_FAULT hides an immediate control-flow jump

`RAISE_FAULT()` sets the flag and immediately jumps to the end of the current
`SWITCH`. Code after it in that switch body is unreachable. It does not stop
the task or select a recovery state, so an unchanged invalid/current case can
raise the fault again on later turns.

**Impact:** perform required actions before raising the fault. In particular,
when both are needed, use `STOP_SELF(); RAISE_FAULT();`; reversing the order
never reaches `STOP_SELF()`.

**Planned resolution:** rename or redesign the operation so the exit behavior
is explicit and combined fault handling cannot depend on surprising order.

### Lifecycle commands do not report rejection

Stop is accepted only in `LOOP`; start is accepted only in `STOPPED`. Calls in
other states silently do nothing, and the commands return no acceptance status.
`TASK_RUNNING` cannot prove that stop is legal because it is also true during
`GUARD`, `ENTRY`, and `STOP`.

**Impact:** check `OS::Tasks::name.getState() == TaskState::LOOP` before stop,
wait for `TASK_RUNNING(name) == false`, and only then start the same task. Do
not use rejected calls as application control flow.

**Planned resolution:** preserve the existing stop-wait-start lifecycle while
adding a precise predicate and a diagnostic or result for rejected commands.

## Context safety

### Context casts are unchecked

Task contexts are stored as `void*`. `CTX` and `TASK_CONTEXT` cast to the type
provided by the caller without validation. A mismatched type is undefined
behavior and may not be detected by ASan/UBSan. `TASK_CONTEXT` on a
context-less task dereferences null.

**Impact:** every access must use exactly the registered type, and
`TASK_CONTEXT` must never be used for a context-less task.

**Planned resolution:** introduce typed handles or type tokens and reject
context-less access at compile time or with an explicit diagnostic.

## Timer behavior

### Timer starts expired and has no disarm operation

A default-constructed `Timer` reports expired before its first `set()`.
`isExpired()` mutates the timer by setting its period to zero after expiry and
then continues to return true until another `set()`. Remaining expired after a
deadline is reached is intentional behavior in this version; the limitation is
that there is no distinct initial/disarmed state or disarm API.

**Impact:** call `set()` before the first query when an initial delay is needed,
and rearm explicitly when a new deadline is needed.

**Planned resolution:** decide whether a future API needs a distinct disarmed
state without changing the current latched-expiry contract silently.

## Name collisions

### Macro names and generated labels may collide

The macro API and global `Task` / `TaskState` names may collide with other
code. `CASES` intentionally creates an unscoped enum for the compact DSL, so
equal state names collide within one translation unit. A handler can contain
only one macro `SWITCH` because the generated `switchEnd_` label is
function-scoped.

**Impact:** keep state names unique within a translation unit, use one macro
`SWITCH` per handler, and check integrations for macro/type collisions.

**Planned resolution:** introduce prefixed public names and remove the fixed-
label limitation while preserving a compact state-machine DSL.
