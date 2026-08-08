#!/usr/bin/env bash
# Compile-only Task layout checks for RV32 and ARM32. No target sysroot,
# linker, emulator, or hardware is required.
set -euo pipefail

CXX="${1:-clang++}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$SCRIPT_DIR/.build/task-size-32"
STD_SHIM="$SCRIPT_DIR/freestanding_std"
SOURCE="$SCRIPT_DIR/test_task_size_32.cpp"

mkdir -p "$BUILD_DIR"

COMMON_FLAGS=(
    -std=c++17
    -ffreestanding
    -fno-exceptions
    -fno-rtti
    -nostdinc++
    -Wall
    -Wextra
    -Werror
    -Wno-unused-private-field
    -I"$STD_SHIM"
    -I"$REPO_ROOT"
    -c
)

echo "=== Task size: CH32V103/V20x class (RV32IMAC / ILP32) ==="
"$CXX" --target=riscv32-none-elf -march=rv32imac -mabi=ilp32 \
    "${COMMON_FLAGS[@]}" "$SOURCE" -o "$BUILD_DIR/task-rv32.o"

echo "=== Task size: CH32V003 class (RV32EC / ILP32E) ==="
"$CXX" --target=riscv32-none-elf -march=rv32ec -mabi=ilp32e \
    "${COMMON_FLAGS[@]}" "$SOURCE" -o "$BUILD_DIR/task-rv32e.o"

echo "=== Task size: STM32F1/CH32F103 class (Cortex-M3 / ARM EABI) ==="
"$CXX" --target=armv7m-none-eabi -mcpu=cortex-m3 -mthumb \
    "${COMMON_FLAGS[@]}" "$SOURCE" -o "$BUILD_DIR/task-arm32.o"

echo "Task is 36 bytes on all three checked 32-bit targets"
