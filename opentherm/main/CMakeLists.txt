# ── The substrate every layer stands on: streams, JSON, the request/reply seam, date
# and time. NOT part of the framework: nothing in here knows about a layer; that is the
# test for whether a thing belongs.
set(LIB_SOURCES
    "lib/system/DateTime.cpp"
    "lib/system/TimeSpan.cpp"
)

# ── The framework. Pulled from the Strux template; a fork should not need to touch
# this list, which is why it is kept apart from the application's.
set(STRUX_SOURCES
    "strux/CommandManager/CommandManager.cpp"
    "strux/SettingsManager/SettingsManager.cpp"
    "strux/SettingsManager/SettingsStore.cpp"
    "strux/SettingsManager/Setting.cpp"
    "strux/SystemManager/SystemManager.cpp"
    "strux/TimeManager/TimeManager.cpp"
)

# ── The application. This is the list a fork owns.
set(APP_SOURCES
    "app/LinkManager/LinkManager.cpp"
    "app/ModemManager/ModemManager.cpp"
)

# ── The ESP-IDF compatibility layer. lib/ is the gateway's code, unchanged, and still
# speaks ESP-IDF where it touches the platform: esp_log, esp_restart. platform/
# answers those on this chip, so the code shared with the gateway stays identical and
# can be diffed against it.
set(PLATFORM_SOURCES
    "platform/Platform.cpp"
)

# ── Board selection ────────────────────────────────────────────
# Pick the target board with -DBOARD=<name>; defaults to the PCB1246. Each board lives
# in main/hardware/boards/<name>/ with its own BoardConfig.h, BoardContext and a
# board.cmake fragment that names the chip: CPU flags, startup code and linker script
# (see that file for the variables it sets).
set(BOARD "pcb1246" CACHE STRING "Target board (folder under main/hardware/boards/)")

set(BOARD_DIR "${CMAKE_CURRENT_LIST_DIR}/hardware/boards/${BOARD}")
if(NOT EXISTS "${BOARD_DIR}/board.cmake")
    file(GLOB _available_boards RELATIVE "${CMAKE_CURRENT_LIST_DIR}/hardware/boards"
         "${CMAKE_CURRENT_LIST_DIR}/hardware/boards/*")
    message(FATAL_ERROR "Unknown BOARD '${BOARD}'. Available boards: ${_available_boards}")
endif()

set(BOARD_SOURCES "")
set(BOARD_INCLUDE_DIRS "")
set(BOARD_COMPILE_OPTIONS "")
set(BOARD_LINK_OPTIONS "")
set(BOARD_CHIP "")
include("${BOARD_DIR}/board.cmake")
message(STATUS "Building for BOARD=${BOARD}")

# ── The firmware ──────────────────────────────────────────────
set(FIRMWARE opentherm)

add_executable(${FIRMWARE}
    "main.cpp"
    ${LIB_SOURCES}
    ${STRUX_SOURCES}
    ${APP_SOURCES}
    ${PLATFORM_SOURCES}
    ${BOARD_SOURCES}
)
set_target_properties(${FIRMWARE} PROPERTIES
    SUFFIX ".elf"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

# Every folder is on the include path, so headers are included by name alone
# (`#include "Stream.h"`) and moving one between layers does not touch its callers.
# The selected board folder is the ONLY board on it, so headers included by name
# (BoardContext.h, BoardConfig.h) resolve to it.
target_include_directories(${FIRMWARE} PRIVATE
    .
    "lib"
    "lib/common"
    "lib/json"
    "lib/protocol"
    "lib/system"
    "strux"
    "strux/CommandManager"
    "strux/SettingsManager"
    "strux/SystemManager"
    "strux/TimeManager"
    "app"
    "app/LinkManager"
    "app/ModemManager"
    "platform"
    "hardware"
    "hardware/boards/${BOARD}"
    ${BOARD_INCLUDE_DIRS}
)

# Board identity define for the (rare) code-level conditionals, and what SystemManager
# reports about the build (ESP-IDF's esp_app_desc on the gateway).
string(TOUPPER "${BOARD}" BOARD_UPPER)
target_compile_definitions(${FIRMWARE} PRIVATE
    # ESP-IDF builds with this too, so the shared code sees the same libc declarations
    # (strptime, for one). newlib-nano declares strptime but does not implement it:
    # DateTime/TimeSpan parsing links only if something calls it, which nothing here does.
    _GNU_SOURCE
    BOARD_${BOARD_UPPER}
    STRUX_PROJECT_NAME="${PROJECT_NAME}"
    STRUX_PROJECT_VER="${PROJECT_VER}"
    STRUX_CHIP="${BOARD_CHIP}"
)

target_compile_options(${FIRMWARE} PRIVATE
    ${BOARD_COMPILE_OPTIONS}
    -Os -g3
    # Switches over enums (e.g. SettingType) deliberately have no default case so
    # that adding an enum value flags every converter that wasn't updated.
    $<$<COMPILE_LANGUAGE:CXX>:-Werror=switch>
)

target_link_options(${FIRMWARE} PRIVATE
    ${BOARD_LINK_OPTIONS}
    "-Wl,-Map=${CMAKE_BINARY_DIR}/${FIRMWARE}.map"
    -Wl,--print-memory-usage
)

# The LPC boot ROM only starts an image whose vector table checksums to zero, so it is
# patched into the ELF first. Then the .bin, whose length and CRC go into the image
# info (ImageInfo.c) of both the .bin and the ELF, and only then the .hex, so all three
# carry the same image.
find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_custom_command(TARGET ${FIRMWARE} POST_BUILD
    COMMAND Python3::Interpreter "${PROJECT_SOURCE_DIR}/tools/lpc_checksum.py" "$<TARGET_FILE:${FIRMWARE}>"
    COMMAND "${CMAKE_OBJCOPY}" -O binary "$<TARGET_FILE:${FIRMWARE}>" "${CMAKE_BINARY_DIR}/${FIRMWARE}.bin"
    COMMAND Python3::Interpreter "${PROJECT_SOURCE_DIR}/tools/lpc_image_info.py" "$<TARGET_FILE:${FIRMWARE}>" "${CMAKE_BINARY_DIR}/${FIRMWARE}.bin"
    COMMAND "${CMAKE_OBJCOPY}" -O ihex "$<TARGET_FILE:${FIRMWARE}>" "${CMAKE_BINARY_DIR}/${FIRMWARE}.hex"
    COMMAND "${CMAKE_SIZE}" "$<TARGET_FILE:${FIRMWARE}>"
    VERBATIM)
