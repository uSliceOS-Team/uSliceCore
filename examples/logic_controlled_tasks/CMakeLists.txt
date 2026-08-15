cmake_minimum_required(VERSION 3.20)

project(uslice_codegen_example LANGUAGES C CXX)

set(REPOSITORY_ROOT "${CMAKE_CURRENT_LIST_DIR}/../..")
set(TASK_DSL "${CMAKE_CURRENT_LIST_DIR}/Tasks.uslice")
set(GENERATED_DIR "${CMAKE_CURRENT_LIST_DIR}/generated")
set(GENERATED_API "${GENERATED_DIR}/Tasks.generated.hpp")

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/main.c" AND
   EXISTS "${CMAKE_CURRENT_LIST_DIR}/main.cpp")
    message(FATAL_ERROR "Provide either main.c or main.cpp, not both")
elseif(EXISTS "${CMAKE_CURRENT_LIST_DIR}/main.c")
    set(APPLICATION_MAIN "${CMAKE_CURRENT_LIST_DIR}/main.c")
    set(GENERATED_MANAGER "${GENERATED_DIR}/Manager.generated.h")
else()
    set(APPLICATION_MAIN "${CMAKE_CURRENT_LIST_DIR}/main.cpp")
    set(GENERATED_MANAGER "${GENERATED_DIR}/Manager.generated.hpp")
endif()

get_filename_component(GENERATED_MANAGER_NAME "${GENERATED_MANAGER}" NAME)
set(GENERATED_DEFINITIONS
    "${GENERATED_DIR}/Tasks.generated.cpp"
)

if(WIN32)
    find_program(USLICE_POWERSHELL_EXECUTABLE NAMES pwsh powershell REQUIRED)
    set(TASKGEN_COMMAND
        "${USLICE_POWERSHELL_EXECUTABLE}"
        -NoProfile
        -ExecutionPolicy
        Bypass
        -File
        "${REPOSITORY_ROOT}/tools/taskgen.ps1"
    )
    set(TASKGEN_SCRIPT "${REPOSITORY_ROOT}/tools/taskgen.ps1")
    set(TASKGEN_ARGUMENTS
        -InputPath "${TASK_DSL}"
        -Api "${GENERATED_API}"
        -Manager "${GENERATED_MANAGER}"
        -Definitions "${GENERATED_DEFINITIONS}"
        -ApiInclude generated/Tasks.generated.hpp
        -ManagerInclude "generated/${GENERATED_MANAGER_NAME}"
        -MainPath "${APPLICATION_MAIN}"
    )
else()
    find_program(USLICE_BASH_EXECUTABLE NAMES bash REQUIRED)
    set(TASKGEN_COMMAND
        "${USLICE_BASH_EXECUTABLE}"
        "${REPOSITORY_ROOT}/tools/taskgen.sh"
    )
    set(TASKGEN_SCRIPT "${REPOSITORY_ROOT}/tools/taskgen.sh")
    set(TASKGEN_ARGUMENTS
        "${TASK_DSL}"
        --api "${GENERATED_API}"
        --manager "${GENERATED_MANAGER}"
        --definitions "${GENERATED_DEFINITIONS}"
        --api-include generated/Tasks.generated.hpp
        --manager-include "generated/${GENERATED_MANAGER_NAME}"
        --main "${APPLICATION_MAIN}"
    )
endif()

add_custom_command(
    OUTPUT
        "${GENERATED_API}"
        "${GENERATED_MANAGER}"
        "${GENERATED_DEFINITIONS}"
    COMMAND
        ${TASKGEN_COMMAND}
        ${TASKGEN_ARGUMENTS}
    DEPENDS
        "${TASK_DSL}"
        "${TASKGEN_SCRIPT}"
        "${CMAKE_CURRENT_LIST_DIR}/TaskDefinitions.hpp"
    COMMENT "Generating the compile-time task registry"
    VERBATIM
)

add_custom_target(generate_example_tasks
    DEPENDS "${GENERATED_API}" "${GENERATED_MANAGER}" "${GENERATED_DEFINITIONS}"
)

add_executable(uslice_codegen_example
    LedBlinker.cpp
    Logic.cpp
    Motor.cpp
    SensorMonitor.cpp
    "${APPLICATION_MAIN}"
    "${GENERATED_API}"
    "${GENERATED_MANAGER}"
    "${GENERATED_DEFINITIONS}"
)

add_dependencies(uslice_codegen_example generate_example_tasks)

target_compile_features(uslice_codegen_example PRIVATE cxx_std_20)
target_include_directories(uslice_codegen_example PRIVATE
    "${REPOSITORY_ROOT}"
    "${CMAKE_CURRENT_LIST_DIR}"
)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(uslice_codegen_example PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Werror
        -Wconversion
        -Wsign-conversion
        -Wshadow
        -Wundef
        -Wformat=2
        -Wswitch-enum
    )
endif()
