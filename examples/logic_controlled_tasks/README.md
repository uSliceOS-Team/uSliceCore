# Example: generated task registration

This host-side example is a working prototype of generated application
registration. [`Tasks.uslice`](Tasks.uslice) is the source of truth. The
generator creates application-specific source and header files:

- `generated/Tasks.generated.hpp` — stable declarations for IDE indexing and
  ordinary application code;
- `generated/Manager.generated.hpp` or `.h` — manager declarations selected by
  the application's `main.cpp` or `main.c`;
- `generated/Tasks.generated.cpp` — `constinit` task definitions,
  `constexpr` linked-list nodes, the one internal registry, and the manager
  implementation.

The generated C++ translation unit is compiled normally and should be included
by exactly one application target. Generated files are committed with this
example so a checkout contains a buildable application; CMake or a direct
generator run can recreate them after changes or removal. The library does not
include any application file.

The DSL is intentionally convention-based:

```text
namespace example::tasks
main "main.cpp"
definitions "TaskDefinitions.hpp"

task ledBlinker autostart
task motor
task sensorMonitor autostart
task logic autostart
```

For `motor`, the generator expects `MotorContext` and generates declarations
and references for `Entry_motor`, `Loop_motor`, and `Stop_motor`. Consequently,
`TaskDefinitions.hpp` contains only context types; handler declarations and all
task objects are generated. `autostart` is an optional flag. The order of task
lines is the scheduler order, so there is no separate registry block. The
registry is private to the generated translation unit and is not passed to the
manager.

`Logic.cpp` includes the generated public header and therefore gets normal
completion and type checking for expressions such as:

```cpp
example::tasks::motorContext.targetSpeed = 42;
example::tasks::motor.start();
```

A build regenerates the files when the DSL, context definitions, or generator
changes. If they are removed, the next build or direct generator run recreates
them. Run the generator directly when an IDE needs the generated declarations
before the first build. CI checks that a fresh generation is deterministic with
`--check`.

## CMake build

From the repository root:

```sh
cmake -S examples/logic_controlled_tasks -B build/codegen-example
cmake --build build/codegen-example
./build/codegen-example/uslice_codegen_example
```

## Direct generation and compilation

```sh
bash tools/taskgen.sh \
  examples/logic_controlled_tasks/Tasks.uslice \
  --api examples/logic_controlled_tasks/generated/Tasks.generated.hpp \
  --manager examples/logic_controlled_tasks/generated/Manager.generated.hpp \
  --definitions examples/logic_controlled_tasks/generated/Tasks.generated.cpp \
  --api-include generated/Tasks.generated.hpp \
  --manager-include generated/Manager.generated.hpp \
  --main main.cpp
```

On Windows, run the equivalent native PowerShell generator:

```powershell
pwsh -NoProfile -File tools/taskgen.ps1 `
  -InputPath examples/logic_controlled_tasks/Tasks.uslice `
  -Api examples/logic_controlled_tasks/generated/Tasks.generated.hpp `
  -Manager examples/logic_controlled_tasks/generated/Manager.generated.hpp `
  -Definitions examples/logic_controlled_tasks/generated/Tasks.generated.cpp `
  -ApiInclude generated/Tasks.generated.hpp `
  -ManagerInclude generated/Manager.generated.hpp `
  -MainPath main.cpp
```

```sh
g++ -std=gnu++20 -Wall -Wextra -Wpedantic \
  -I. -Iexamples/logic_controlled_tasks \
  examples/logic_controlled_tasks/LedBlinker.cpp \
  examples/logic_controlled_tasks/Motor.cpp \
  examples/logic_controlled_tasks/SensorMonitor.cpp \
  examples/logic_controlled_tasks/Logic.cpp \
  examples/logic_controlled_tasks/generated/Tasks.generated.cpp \
  examples/logic_controlled_tasks/main.cpp \
  -o example

./example
```

Check generated artifacts without rewriting them. Generate them once first if
the files were removed:

```sh
bash tools/taskgen.sh \
  examples/logic_controlled_tasks/Tasks.uslice \
  --api examples/logic_controlled_tasks/generated/Tasks.generated.hpp \
  --manager examples/logic_controlled_tasks/generated/Manager.generated.hpp \
  --definitions examples/logic_controlled_tasks/generated/Tasks.generated.cpp \
  --api-include generated/Tasks.generated.hpp \
  --manager-include generated/Manager.generated.hpp \
  --check
```

For a plain-C `main`, set `main "main.c"` in the DSL (or pass `--main`) and
include the generated C manager header. The call does not expose the registry:

```c
#include "generated/Manager.generated.h"

int main(void) {
    usliceTaskManager();
}
```
