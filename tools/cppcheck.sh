#!/usr/bin/env bash
# Static analysis for µSliceCore core (tasks/, time/, uSliceCore.h).
#
# Usage:
#   tools/cppcheck.sh            # standard checks
#   tools/cppcheck.sh --misra    # also run the MISRA addon
#
# The MISRA addon works without any extra setup: it reports rule IDs for
# violations it finds. If you have a purchased MISRA standard and want
# descriptive text alongside each finding, copy
# tools/misra_rule_texts.example.txt to tools/misra_rule_texts.txt and fill
# it in from your own copy of the standard -- that file is gitignored and
# must never be committed (see docs/MISRA_COMPLIANCE.md for why).

set -euo pipefail
cd "$(dirname "$0")/.."

SOURCES=(tasks time uSliceCore.h)
COMMON_FLAGS=(
    --enable=warning,style,performance,portability
    --inconclusive
    --std=c++20
    -D__cpp_consteval=201811L
    --language=c++
    --error-exitcode=1
    --inline-suppr
    --suppress=missingIncludeSystem
)

if [[ "${1:-}" == "--misra" ]]; then
    MISRA_ARGS=(--addon=misra)
    if [[ -f tools/misra_rule_texts.txt ]]; then
        echo "Using local MISRA rule texts (not committed)."
        # cppcheck's misra addon reads this via an addon config; see cppcheck
        # docs for the current --addon-config mechanism in your installed
        # version, since the flag has changed across releases.
    else
        echo "No tools/misra_rule_texts.txt found -- MISRA findings will show" \
            "rule IDs only, no descriptive text. See tools/misra_rule_texts.example.txt."
    fi
    cppcheck "${COMMON_FLAGS[@]}" "${MISRA_ARGS[@]}" "${SOURCES[@]}"
else
    cppcheck "${COMMON_FLAGS[@]}" "${SOURCES[@]}"
fi
