#!/usr/bin/env bash
# Builds and runs every host-side test in this folder, one binary per
# scenario (see README.md in this folder for why). Exits 0 only if every
# test binary exits 0 -- safe to wire into CI as a gate before flashing
# real hardware.
#
# Usage: ./run_tests.sh [path-to-g++] [path-to-gcc]
set -euo pipefail

CXX="${1:-g++}"
CC="${2:-gcc}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
COMPILER_NAME="$(basename "$CXX")"
BUILD_DIR="$SCRIPT_DIR/.build/$COMPILER_NAME"

mkdir -p "$BUILD_DIR"

CXXFLAGS=(
    -std=gnu++20
    -Wall
    -Wextra
    -Wpedantic
    -Werror
    -Wconversion
    -Wsign-conversion
    -Wshadow
    -Wundef
    -Wcast-align
    -Wcast-qual
    -Wformat=2
    -Wnull-dereference
    -Wdouble-promotion
    -Wimplicit-fallthrough
    -Wswitch-enum
    -I"$REPO_ROOT"
    -I"$SCRIPT_DIR"
)

if [[ "${USLICE_SANITIZERS:-0}" == "1" ]]; then
    CXXFLAGS+=(
        -fno-omit-frame-pointer
        "-fsanitize=address,undefined"
    )
fi

CFLAGS=(
    -std=c11
    -Wall
    -Wextra
    -Wpedantic
    -Werror
    -Wconversion
    -Wsign-conversion
    -Wshadow
    -Wundef
    -Wcast-align
    -Wcast-qual
    -Wformat=2
    -fno-common
    -I"$REPO_ROOT"
)

overall_status=0
total=0
passed=0

run_one() {
    local test_name="$1"
    shift
    local sources=("$@")
    total=$((total + 1))

    local bin="$BUILD_DIR/$test_name"
    echo "=== $test_name ==="

    if ! "$CXX" "${CXXFLAGS[@]}" "${sources[@]}" -o "$bin" \
        2>"$BUILD_DIR/$test_name.build.log"; then
        echo "  BUILD FAILED -- see $BUILD_DIR/$test_name.build.log"
        overall_status=1
        return
    fi

    if "$bin"; then
        passed=$((passed + 1))
    else
        overall_status=1
    fi
    echo
}

run_c_api_test() {
    local test_name="test_c_api"
    local c_obj="$BUILD_DIR/$test_name.c.o"
    local bin="$BUILD_DIR/$test_name"
    total=$((total + 1))

    echo "=== $test_name ==="
    if ! "$CC" "${CFLAGS[@]}" \
        -c "$SCRIPT_DIR/test_c_api.c" -o "$c_obj" \
        2>"$BUILD_DIR/$test_name.build.log"; then
        echo "  C11 BUILD FAILED -- see $BUILD_DIR/$test_name.build.log"
        overall_status=1
        return
    fi

    if ! "$CXX" "${CXXFLAGS[@]}" "$SCRIPT_DIR/test_c_api.cpp" "$c_obj" \
        "$REPO_ROOT/time/osTickISR.cpp" -o "$bin" \
        2>>"$BUILD_DIR/$test_name.build.log"; then
        echo "  MIXED LINK BUILD FAILED -- see $BUILD_DIR/$test_name.build.log"
        overall_status=1
        return
    fi

    if "$bin"; then
        passed=$((passed + 1))
    else
        overall_status=1
    fi
    echo
}

run_c_generated_registry_test() {
    local test_name="test_c_generated_main"
    local c_obj="$BUILD_DIR/$test_name.c.o"
    local bin="$BUILD_DIR/$test_name"
    local generated_dir="$BUILD_DIR/$test_name.generated"
    total=$((total + 1))

    echo "=== $test_name ==="
    if ! bash "$REPO_ROOT/tools/taskgen.sh" \
        "$REPO_ROOT/examples/logic_controlled_tasks/Tasks.uslice" \
        --main "$SCRIPT_DIR/$test_name.c" \
        --api "$generated_dir/Tasks.generated.hpp" \
        --manager "$generated_dir/generated_manager.h" \
        --definitions "$generated_dir/Tasks.generated.cpp" \
        --api-include Tasks.generated.hpp \
        --manager-include generated_manager.h \
        2>"$BUILD_DIR/$test_name.build.log"; then
        echo "  GENERATOR FAILED -- see $BUILD_DIR/$test_name.build.log"
        overall_status=1
        return
    fi

    if ! "$CC" "${CFLAGS[@]}" \
        -I"$generated_dir" \
        -c "$SCRIPT_DIR/$test_name.c" -o "$c_obj" \
        2>>"$BUILD_DIR/$test_name.build.log"; then
        echo "  C11 BUILD FAILED -- see $BUILD_DIR/$test_name.build.log"
        overall_status=1
        return
    fi

    if ! "$CXX" "${CXXFLAGS[@]}" \
        -I"$generated_dir" \
        -I"$REPO_ROOT/examples/logic_controlled_tasks" "$c_obj" \
        "$REPO_ROOT/examples/logic_controlled_tasks/LedBlinker.cpp" \
        "$REPO_ROOT/examples/logic_controlled_tasks/Logic.cpp" \
        "$REPO_ROOT/examples/logic_controlled_tasks/Motor.cpp" \
        "$REPO_ROOT/examples/logic_controlled_tasks/SensorMonitor.cpp" \
        "$generated_dir/Tasks.generated.cpp" -o "$bin" \
        2>>"$BUILD_DIR/$test_name.build.log"; then
        echo "  MIXED LINK BUILD FAILED -- see $BUILD_DIR/$test_name.build.log"
        overall_status=1
        return
    fi

    if "$bin"; then
        passed=$((passed + 1))
    else
        overall_status=1
    fi
    echo
}

run_compile_fail_test() {
    local test_name="compile_fail_missing_loop"
    total=$((total + 1))

    echo "=== $test_name ==="
    if "$CXX" "${CXXFLAGS[@]}" -c \
        "$SCRIPT_DIR/$test_name.cpp" -o "$BUILD_DIR/$test_name.o" \
        2>"$BUILD_DIR/$test_name.build.log"; then
        echo "  UNEXPECTEDLY COMPILED -- missing loop must be rejected"
        overall_status=1
        return
    fi
    if ! grep -q "rejectMissingLoopHandler" \
        "$BUILD_DIR/$test_name.build.log"; then
        echo "  FAILED FOR THE WRONG REASON -- see the build log"
        overall_status=1
        return
    fi
    passed=$((passed + 1))
    echo "  rejected as required"
    echo
}

run_one test_lifecycle_autostart       "$SCRIPT_DIR/test_lifecycle_autostart.cpp"
run_one test_lifecycle_manual_start    "$SCRIPT_DIR/test_lifecycle_manual_start.cpp"
run_one test_self_stop_and_cleanup     "$SCRIPT_DIR/test_self_stop_and_cleanup.cpp"
run_one test_fault_flag                "$SCRIPT_DIR/test_fault_flag.cpp"
run_one test_task_instance             "$SCRIPT_DIR/test_task_instance.cpp"
run_one test_timer                     "$SCRIPT_DIR/test_timer.cpp"
run_one test_restart_after_cleanup     "$SCRIPT_DIR/test_restart_after_cleanup.cpp"
run_one test_cross_task_handover       "$SCRIPT_DIR/test_cross_task_handover.cpp"
run_one test_contextless_and_registration \
    "$SCRIPT_DIR/test_contextless_and_registration.cpp"
run_one test_cross_file_declare_and_context \
    "$SCRIPT_DIR/test_cross_file_main.cpp" "$SCRIPT_DIR/test_cross_file_task.cpp"
run_c_api_test
run_c_generated_registry_test
run_compile_fail_test

echo "============================================"
echo "$passed / $total test binaries passed"
exit $overall_status
