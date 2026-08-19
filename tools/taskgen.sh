#!/usr/bin/env bash
# Generate one application task registry and its main-specific manager API.
# The PowerShell implementation in taskgen.ps1 emits the same files.

set -euo pipefail
shopt -s extglob

usage() {
    cat >&2 <<'EOF'
Usage:
  taskgen.sh INPUT --api PATH --definitions PATH --manager PATH
              [--main PATH] [--api-include NAME] [--manager-include NAME]
              [--check]

The main file is taken from --main, then from main "..." in INPUT, and
finally from a unique main.c or main.cpp next to INPUT.
EOF
}

fail() {
    printf 'taskgen: %s\n' "$*" >&2
    exit 2
}

input_path=''
api_path=''
definitions_path=''
manager_path=''
main_override=''
api_include=''
manager_include=''
check=0

if (($# == 0)); then
    usage
    exit 2
fi

input_path=$1
shift
while (($# > 0)); do
    case $1 in
        --api|--definitions|--manager|--main|--api-include|--manager-include)
            (($# >= 2)) || fail "missing value for $1"
            case $1 in
                --api) api_path=$2 ;;
                --definitions) definitions_path=$2 ;;
                --manager) manager_path=$2 ;;
                --main) main_override=$2 ;;
                --api-include) api_include=$2 ;;
                --manager-include) manager_include=$2 ;;
            esac
            shift 2
            ;;
        --check)
            check=1
            shift
            ;;
        --help|-h)
            usage
            exit 0
            ;;
        *)
            usage
            fail "unknown argument '$1'"
            ;;
    esac
done

[[ -n $api_path ]] || fail '--api is required'
[[ -n $definitions_path ]] || fail '--definitions is required'
[[ -n $manager_path ]] || fail '--manager is required'
[[ -f $input_path ]] || fail "input file not found: $input_path"

source_name=${input_path##*/}
input_dir=$(cd "$(dirname "$input_path")" && pwd)

namespace=''
main_from_dsl=''
definitions=''
declare -a tasks=()
declare -a autostarts=()
declare -a case_lists=()
current_task_index=-1
current_task_line=0

trim() {
    local value=$1
    value=${value##+([[:space:]])}
    value=${value%%+([[:space:]])}
    printf '%s' "$value"
}

line_number=0
while IFS= read -r raw_line || [[ -n $raw_line ]]; do
    line_number=$((line_number + 1))
    line=${raw_line%%#*}
    line=$(trim "$line")
    [[ -n $line ]] || continue

    if ((current_task_index >= 0)); then
        if [[ $line == '}' ]]; then
            current_task_index=-1
        elif [[ $line =~ ^case[[:space:]]+([A-Za-z_][A-Za-z0-9_]*)$ ]]; then
            case_name=${BASH_REMATCH[1]}
            [[ " ${case_lists[current_task_index]} " != *" $case_name "* ]] ||
                fail "$input_path:$line_number: duplicate case '$case_name' for task '${tasks[current_task_index]}'"
            case_lists[current_task_index]+="$case_name "
        else
            fail "$input_path:$line_number: unexpected statement '$line' in task '${tasks[current_task_index]}'"
        fi
    elif [[ $line =~ ^namespace[[:space:]]+(.+)$ ]]; then
        [[ -z $namespace ]] || fail "$input_path:$line_number: namespace may be declared only once"
        namespace=$(trim "${BASH_REMATCH[1]}")
        [[ $namespace =~ ^(::)?[A-Za-z_][A-Za-z0-9_]*(::[A-Za-z_][A-Za-z0-9_]*)*$ ]] ||
            fail "$input_path:$line_number: invalid namespace '$namespace'"
    elif [[ $line =~ ^main[[:space:]]+\"([^\"]+)\"$ ]]; then
        [[ -z $main_from_dsl ]] || fail "$input_path:$line_number: main may be declared only once"
        main_from_dsl=${BASH_REMATCH[1]}
    elif [[ $line =~ ^definitions[[:space:]]+\"([^\"]+)\"$ ]]; then
        [[ -z $definitions ]] || fail "$input_path:$line_number: definitions may be declared only once"
        definitions=${BASH_REMATCH[1]}
    elif [[ $line =~ ^task[[:space:]]+([A-Za-z_][A-Za-z0-9_]*)([[:space:]]+with[[:space:]]+autostart)?[[:space:]]*[{]$ ]]; then
        task_name=${BASH_REMATCH[1]}
        for existing in "${tasks[@]}"; do
            [[ $existing != "$task_name" ]] ||
                fail "$input_path:$line_number: duplicate task '$task_name'"
        done
        tasks+=("$task_name")
        case_lists+=('')
        if [[ -n ${BASH_REMATCH[2]} ]]; then
            autostarts+=(true)
        else
            autostarts+=(false)
        fi
        current_task_index=$((${#tasks[@]} - 1))
        current_task_line=$line_number
    else
        fail "$input_path:$line_number: unexpected statement '$line'"
    fi
done < "$input_path"

((current_task_index < 0)) ||
    fail "$input_path:$current_task_line: unclosed task '${tasks[current_task_index]}'"
[[ -n $namespace ]] || fail "$input_path: namespace is required"
[[ -n $definitions ]] || fail "$input_path: definitions header is required"
declare -a parsed_cases=()
for task_index in "${!tasks[@]}"; do
    [[ -n ${case_lists[task_index]} ]] ||
        fail "$input_path: task '${tasks[task_index]}' must declare at least one case"
    read -r -a parsed_cases <<< "${case_lists[task_index]}"
    ((${#parsed_cases[@]} <= 256)) ||
        fail "$input_path: task '${tasks[task_index]}' exceeds the 256-case limit"
done

main_path=$main_override
if [[ -z $main_path ]]; then
    main_path=$main_from_dsl
fi
if [[ -z $main_path ]]; then
    declare -a candidates=()
    [[ -f "$input_dir/main.c" ]] && candidates+=("$input_dir/main.c")
    [[ -f "$input_dir/main.cpp" ]] && candidates+=("$input_dir/main.cpp")
    ((${#candidates[@]} == 1)) || {
        if ((${#candidates[@]} == 0)); then
            fail "$input_path: main file was not found; use --main or main \"...\""
        fi
        fail "$input_path: both main.c and main.cpp exist; use --main or main \"...\""
    }
    main_path=${candidates[0]}
elif [[ $main_path != /* ]]; then
    main_path="$input_dir/$main_path"
fi

[[ -f $main_path ]] || fail "main file not found: $main_path"
main_name=${main_path##*/}
case ${main_name,,} in
    *.c) mode=c ;;
    *.cc|*.cpp|*.cxx) mode=cpp ;;
    *) fail "unsupported main extension in '$main_name'; expected .c or .cpp" ;;
esac

api_include=${api_include:-${api_path##*/}}
manager_include=${manager_include:-${manager_path##*/}}
cpp_namespace=${namespace#::}

type_stem() {
    local task=$1
    local first=${task:0:1}
    printf '%s%s' "${first^^}" "${task:1}"
}

render_api() {
    printf '%s\n' '// Generated by tools/taskgen.sh or tools/taskgen.ps1; do not edit.'
    printf '// Source: %s\n' "$source_name"
    printf '// Main: %s (%s)\n' "$main_name" "$mode"
    printf '%s\n\n' '#pragma once'
    printf '%s\n' '#include <cstdint>'
    printf '%s\n' '#include "tasks/Task.hpp"'
    printf '#include "%s"\n' "$definitions"
    for task in "${tasks[@]}"; do
        printf '\nvoid Loop_%s(void* rawCtx_, ::uslice::Task* self);\n' "$task"
        printf 'void Stop_%s(void* rawCtx_, ::uslice::Task* self);\n' "$task"
    done
    printf '\nnamespace %s {\n\n' "$cpp_namespace"
    for task in "${tasks[@]}"; do
        stem=$(type_stem "$task")
        task_index=0
        while [[ ${tasks[task_index]} != "$task" ]]; do
            task_index=$((task_index + 1))
        done
        read -r -a parsed_cases <<< "${case_lists[task_index]}"
        printf 'enum class %sCase : std::uint8_t {\n' "$stem"
        for case_name in "${parsed_cases[@]}"; do
            printf '    %s,\n' "$case_name"
        done
        printf '};\n\n'
        printf '// Typed view over ::uslice::Task for the %s case enum; replaces\n' "$stem"
        printf '// static_cast<%sCase>(self->currentCase()) and\n' "$stem"
        printf '// self->gotoCase(static_cast<::uslice::Task::case_t>(...)) with\n'
        printf '// type-checked calls, so a wrong-task case value cannot compile.\n'
        printf 'class %sHandle {\n' "$stem"
        printf '    ::uslice::Task* task_;\n\n'
        printf 'public:\n'
        printf '    constexpr explicit %sHandle(::uslice::Task* task) noexcept\n' "$stem"
        printf '        : task_(task) {}\n\n'
        printf '    [[nodiscard]] constexpr %sCase currentCase() const noexcept {\n' "$stem"
        printf '        return static_cast<%sCase>(task_->currentCase());\n' "$stem"
        printf '    }\n'
        printf '    constexpr void gotoCase(%sCase value) const noexcept {\n' "$stem"
        printf '        task_->gotoCase(static_cast<::uslice::Task::case_t>(value));\n'
        printf '    }\n'
        printf '    constexpr void stop() const noexcept { task_->stop(); }\n'
        printf '    constexpr bool start() const noexcept { return task_->start(); }\n'
        printf '    constexpr void raiseFault() const noexcept { task_->raiseFault(); }\n'
        printf '    [[nodiscard]] constexpr bool isFaulted() const noexcept {\n'
        printf '        return task_->isFaulted();\n'
        printf '    }\n'
        printf '    [[nodiscard]] constexpr bool isRunning() const noexcept {\n'
        printf '        return task_->isRunning();\n'
        printf '    }\n'
        printf '    [[nodiscard]] constexpr bool isStopped() const noexcept {\n'
        printf '        return task_->isStopped();\n'
        printf '    }\n'
        printf '    [[nodiscard]] constexpr ::uslice::TaskState state() const noexcept {\n'
        printf '        return task_->state();\n'
        printf '    }\n'
        printf '};\n\n'
        printf '%sContext& %sContext() noexcept;\n' "$stem" "$task"
        printf '::uslice::Task& %s() noexcept;\n\n' "$task"
    done
    printf '} // namespace %s\n' "$cpp_namespace"
}

render_manager() {
    if [[ $mode == c ]]; then
        printf '%s\n' '/* Generated by tools/taskgen.sh or tools/taskgen.ps1; do not edit. */'
        printf '/* Source: %s */\n' "$source_name"
        printf '/* Main: %s (C) */\n' "$main_name"
        printf '%s\n\n' '#pragma once'
        printf '%s\n' '#ifdef __cplusplus'
        printf '%s\n' 'extern "C" {'
        printf '%s\n' '#endif'
        printf '\nvoid usliceTaskManager(void);\nvoid usliceTaskManagerPass(void);\n'
        printf '\n%s\n' '#ifdef __cplusplus'
        printf '%s\n' '}'
        printf '%s\n' '#endif'
    else
        printf '%s\n' '// Generated by tools/taskgen.sh or tools/taskgen.ps1; do not edit.'
        printf '// Source: %s\n' "$source_name"
        printf '// Main: %s (C++)\n' "$main_name"
        printf '%s\n\n' '#pragma once'
        printf '%s\n' 'void usliceTaskManager(void);'
        printf '%s\n' 'void usliceTaskManagerPass(void);'
    fi
}

render_definitions() {
    printf '%s\n' '// Generated by tools/taskgen.sh or tools/taskgen.ps1; compile this translation unit.'
    printf '// Source: %s\n' "$source_name"
    printf '// Main: %s (%s)\n' "$main_name" "$mode"
    printf '#include "%s"\n' "$api_include"
    printf '#include "%s"\n' "$manager_include"
    printf '#include "tasks/Task.hpp"\n'
    printf '#include "%s"\n\n' "$definitions"

    printf 'namespace %s::generated_detail {\n\n' "$cpp_namespace"
    for task_index in "${!tasks[@]}"; do
        task=${tasks[task_index]}
        read -r -a parsed_cases <<< "${case_lists[task_index]}"
        case_count=${#parsed_cases[@]}
        printf 'constexpr ::uslice::Task::Program %sProgram{\n' "$task"
        printf '    .loop = ::Loop_%s,\n' "$task"
        printf '    .stop = ::Stop_%s,\n' "$task"
        printf '    .caseCount = %d,\n' "$case_count"
        printf '};\n\n'
    done
    printf '} // namespace %s::generated_detail\n\n' "$cpp_namespace"

    printf 'namespace {\n\n'
    printf 'template <typename Context>\n'
    printf 'class ContextStorage {\n'
    printf '    mutable Context value{};\n\n'
    printf 'public:\n'
    printf '    constexpr Context& get() const noexcept { return value; }\n'
    printf '};\n\n'
    printf 'template <const ::uslice::Task::Program* ProgramPtr>\n'
    printf 'class TaskStorage {\n'
    printf '    mutable ::uslice::Task value;\n\n'

    printf 'public:\n'
    printf '    consteval explicit TaskStorage(\n'
    printf '        ::uslice::Task::Definition<ProgramPtr> definition) noexcept\n'
    printf '        : value(definition) {}\n'
    printf '    constexpr ::uslice::Task& get() const noexcept { return value; }\n'
    printf '};\n\n'
    for task_index in "${!tasks[@]}"; do
        task=${tasks[task_index]}
        stem=$(type_stem "$task")
        printf 'constinit const ContextStorage<%sContext> %sContextStorage{};\n' "$stem" "$task"
        printf 'constinit const TaskStorage<&%s::generated_detail::%sProgram> %sStorage{\n' \
            "$cpp_namespace" "$task" "$task"
        printf '    ::uslice::Task::Definition<&%s::generated_detail::%sProgram>{\n' \
            "$cpp_namespace" "$task"
        printf '    .context = &%sContextStorage.get(),\n' "$task"
        printf '    .autostart = %s,\n' "${autostarts[task_index]}"
        printf '    }\n};\n\n'
    done
    printf '} // namespace\n\n'

    printf 'namespace %s {\n\n' "$cpp_namespace"
    for task in "${tasks[@]}"; do
        stem=$(type_stem "$task")
        printf '%sContext& %sContext() noexcept { return %sContextStorage.get(); }\n' \
            "$stem" "$task" "$task"
        printf '::uslice::Task& %s() noexcept { return %sStorage.get(); }\n\n' \
            "$task" "$task"
    done
    printf '} // namespace %s\n\n' "$cpp_namespace"

    printf 'namespace %s::generated_detail {\n' "$cpp_namespace"
    printf '\n// Nodes are declared tail-first; traversal remains in DSL order.\n'
    if ((${#tasks[@]} == 0)); then
        printf '// Scheduler order: (empty)\n'
    else
        scheduler_order=${tasks[0]}
        for ((task_index = 1; task_index < ${#tasks[@]}; task_index++)); do
            scheduler_order+=" -> ${tasks[task_index]}"
        done
        printf '// Scheduler order: %s\n' "$scheduler_order"
    fi
    next_link='nullptr'
    for ((task_index=${#tasks[@]} - 1; task_index >= 0; task_index--)); do
        task=${tasks[task_index]}
        link_name="taskRegistry_${task}Link"
        printf 'constexpr ::uslice::TaskLink %s{&%sStorage.get(), %s};\n' \
            "$link_name" "$task" "$next_link"
        next_link="&$link_name"
    done
    printf '\n} // namespace %s::generated_detail\n\n' "$cpp_namespace"

    if [[ $next_link == 'nullptr' ]]; then
        registry_head='nullptr'
    else
        registry_head="&${cpp_namespace}::generated_detail::${next_link#&}"
    fi
    printf 'namespace {\nconstinit const ::uslice::TaskRegistry taskRegistry{%s};\n}\n\n' "$registry_head"

    if [[ $mode == c ]]; then
        printf 'extern "C" void usliceTaskManagerPass(void) {\n'
    else
        printf 'void usliceTaskManagerPass(void) {\n'
    fi
    printf '    taskRegistry.executePass();\n}\n\n'

    if [[ $mode == c ]]; then
        printf 'extern "C" void usliceTaskManager(void) {\n'
    else
        printf 'void usliceTaskManager(void) {\n'
    fi
    printf '    while (true) {\n        taskRegistry.executePass();\n    }\n}\n'
}

update_file() {
    local path=$1
    local renderer=$2
    local directory
    local temporary
    directory=$(dirname "$path")
    mkdir -p "$directory"
    temporary=$(mktemp "$directory/.taskgen.XXXXXX")
    "$renderer" > "$temporary"
    if [[ -f $path ]] && cmp -s "$temporary" "$path"; then
        rm -f "$temporary"
        return 0
    fi
    if ((check)); then
        printf 'outdated generated file: %s\n' "$path" >&2
        rm -f "$temporary"
        return 1
    fi
    mv "$temporary" "$path"
}

update_file "$api_path" render_api
update_file "$manager_path" render_manager
update_file "$definitions_path" render_definitions
