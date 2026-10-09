cmake_minimum_required(VERSION 3.20)

# Cross-compile for the board's Arm core. Must be set before project().
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/arm-none-eabi.cmake")

# ── The version ──────────────────────────────────────────────
# From the git tag (v0.1.0 -> 0.1.0), else 0.0.0-dev. A release candidate builds as
# the version it is a candidate FOR (v1.2.0-rc.2 -> 1.2.0). Same rule as the gateway.
execute_process(
    COMMAND git describe --tags --abbrev=0
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_TAG
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
    RESULT_VARIABLE GIT_TAG_RESULT
)
if(GIT_TAG_RESULT EQUAL 0 AND GIT_TAG MATCHES "^v([0-9]+\\.[0-9]+\\.[0-9]+)(-rc\\.[0-9]+)?$")
    set(PROJECT_VER "${CMAKE_MATCH_1}")
else()
    set(PROJECT_VER "0.0.0-dev")
endif()

project(KC-OT-PCB1246 LANGUAGES C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── The commit ────────────────────────────────────────────────
# Compiled in so a device can say which build it runs; -dirty is part of the answer.
# Empty in a source drop with no .git.
execute_process(
    COMMAND git describe --always --dirty --exclude "*"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
    RESULT_VARIABLE GIT_COMMIT_RESULT
)
if(NOT GIT_COMMIT_RESULT EQUAL 0)
    set(GIT_COMMIT "")
endif()
add_compile_definitions(STRUX_GIT_COMMIT="${GIT_COMMIT}")

set(THIRD_PARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party")

add_subdirectory(main)
