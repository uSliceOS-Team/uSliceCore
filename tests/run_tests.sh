#!/usr/bin/env bash
# Builds and runs every host-side test in this folder, one binary per
# scenario (see README.md in this folder for why). Exits 0 only if every
# test binary exits 0 -- safe to wire into CI as a gate before flashing
# real hardware.
#
# Usage: ./run_tests.sh [path-to-g++] [path-to-gcc]
set -u

CXX="${1:-g++}"
CC="${2:-gcc}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$SCRIPT_DIR/.build"

mkdir -p "$BUILD_DIR"

CXXFLAGS="-std=c++17 -Wall -Wextra -I$REPO_ROOT -I$SCRIPT_DIR"

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

    if ! "$CXX" $CXXFLAGS "${sources[@]}" -o "$bin" 2>"$BUILD_DIR/$test_name.build.log"; then
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
    if ! "$CC" -std=c11 -Wall -Wextra -pedantic -I"$REPO_ROOT" \
        -c "$SCRIPT_DIR/test_c_api.c" -o "$c_obj" \
        2>"$BUILD_DIR/$test_name.build.log"; then
        echo "  C11 BUILD FAILED -- see $BUILD_DIR/$test_name.build.log"
        overall_status=1
        return
    fi

    if ! "$CXX" $CXXFLAGS "$SCRIPT_DIR/test_c_api.cpp" "$c_obj" \
        "$REPO_ROOT/tasks/osTaskManager.cpp" \
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

echo "============================================"
echo "$passed / $total test binaries passed"
exit $overall_status
