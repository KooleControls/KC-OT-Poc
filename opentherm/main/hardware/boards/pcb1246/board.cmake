# ──────────────────────────────────────────────────────────────
# Board fragment: PCB1246
#   The OpenTherm plug-in board · LPC11U68JBD100 (Cortex-M0+), 256 KB flash,
#   32 KB SRAM0, clocked at 48 MHz from the internal 12 MHz IRC
#
# The board is the whole platform here, not just a pinout: it names the CPU,
# the startup code, the clock setup and the linker script.
# main/CMakeLists.txt reads these variables:
#   BOARD_SOURCES          .c/.cpp files under this folder
#   BOARD_INCLUDE_DIRS     extra include directories (this folder is added anyway)
#   BOARD_COMPILE_OPTIONS  CPU flags, for every source file
#   BOARD_LINK_OPTIONS     CPU flags and the linker script
#   BOARD_CHIP             the chip's name, as `system info` reports it
# ──────────────────────────────────────────────────────────────

set(_cpu -mcpu=cortex-m0plus -mthumb)

list(APPEND BOARD_SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/startup_LPC11U68.c"
    "${CMAKE_CURRENT_LIST_DIR}/system_LPC11U6x.c"
    "${CMAKE_CURRENT_LIST_DIR}/ImageInfo.c"
    "${CMAKE_CURRENT_LIST_DIR}/BoardContext.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../../drivers/LpcUart0.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../../drivers/ManchesterLine.cpp"
)

list(APPEND BOARD_INCLUDE_DIRS
    "${THIRD_PARTY_DIR}/LPC11U6x"
    "${THIRD_PARTY_DIR}/CMSIS/Core/Include"
)

list(APPEND BOARD_COMPILE_OPTIONS ${_cpu})

list(APPEND BOARD_LINK_OPTIONS
    ${_cpu}
    "-T${CMAKE_CURRENT_LIST_DIR}/LPC11U68.ld"
)

set(BOARD_CHIP "LPC11U68")
