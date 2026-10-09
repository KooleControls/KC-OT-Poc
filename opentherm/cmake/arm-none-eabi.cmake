# Cross-compile with the Arm GNU Toolchain (arm-none-eabi). Generic Arm: the CPU flags
# are the board's (see main/hardware/boards/<board>/board.cmake), not the toolchain's.
#
# The toolchain is taken from PATH, or else from the default Windows install folder.
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

file(GLOB _arm_gnu_bins "C:/Program Files*/Arm GNU Toolchain arm-none-eabi/*/bin")
find_program(ARM_GCC arm-none-eabi-gcc HINTS ${_arm_gnu_bins} REQUIRED)
get_filename_component(_arm_bin "${ARM_GCC}" DIRECTORY)
# CMAKE_EXECUTABLE_SUFFIX is not known yet when the toolchain file runs.
if(CMAKE_HOST_WIN32)
    set(_exe ".exe")
else()
    set(_exe "")
endif()

set(CMAKE_C_COMPILER   "${_arm_bin}/arm-none-eabi-gcc${_exe}")
set(CMAKE_CXX_COMPILER "${_arm_bin}/arm-none-eabi-g++${_exe}")
set(CMAKE_ASM_COMPILER "${_arm_bin}/arm-none-eabi-gcc${_exe}")
set(CMAKE_OBJCOPY      "${_arm_bin}/arm-none-eabi-objcopy${_exe}" CACHE FILEPATH "")
set(CMAKE_SIZE         "${_arm_bin}/arm-none-eabi-size${_exe}" CACHE FILEPATH "")

# CMake's compiler check would try to link a host-style executable, which fails without
# a linker script.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_FLAGS_INIT   "-ffunction-sections -fdata-sections")
# -fno-threadsafe-statics: single-threaded firmware, so the guard GCC puts around
# function-local statics would only cost code.
set(CMAKE_CXX_FLAGS_INIT "-ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-threadsafe-statics")
set(CMAKE_EXE_LINKER_FLAGS_INIT "--specs=nano.specs --specs=nosys.specs -Wl,--gc-sections")
