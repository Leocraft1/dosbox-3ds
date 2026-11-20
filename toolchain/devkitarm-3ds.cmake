# toolchain/devkitarm-3ds.cmake
#
# CMake toolchain file to cross-compile for the Nintendo 3DS (ARM11 / ARMv6K)
# using devkitARM. This does NOT attempt to drive dosbox-staging's official
# CMakeLists.txt + vcpkg graph (vcpkg has no Horizon/CTR triplet, and several
# upstream dependencies - SDL3, fluidsynth, libmt32emu, opusfile - have no
# 3DS port). Use it to build a dedicated, minimal target (see Makefile.ctr
# precedent in the original fork) that compiles a hand-picked subset of
# dosbox-staging's src/ tree plus the platform/ctr/ glue directly.
#
# Usage:
#   export DEVKITPRO=/opt/devkitpro        # standard devkitPro install path
#   export DEVKITARM=$DEVKITPRO/devkitARM
#   cmake -B build -DCMAKE_TOOLCHAIN_FILE=toolchain/devkitarm-3ds.cmake
#
# Status: scaffold, not yet validated against an actual devkitARM install
# (this environment has no devkitARM toolchain available to test-compile
# against). Verify paths/flags against your local devkitPro install before
# relying on it.

if(NOT DEFINED ENV{DEVKITARM})
  message(FATAL_ERROR "DEVKITARM is not set. export DEVKITARM=<path to>/devkitARM")
endif()
if(NOT DEFINED ENV{DEVKITPRO})
  message(FATAL_ERROR "DEVKITPRO is not set. export DEVKITPRO=<path to>/devkitpro")
endif()

set(DEVKITARM $ENV{DEVKITARM})
set(DEVKITPRO $ENV{DEVKITPRO})

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_CROSSCOMPILING TRUE)

# devkitARM toolchain binaries
set(CMAKE_C_COMPILER   "${DEVKITARM}/bin/arm-none-eabi-gcc")
set(CMAKE_CXX_COMPILER "${DEVKITARM}/bin/arm-none-eabi-g++")
set(CMAKE_ASM_COMPILER "${DEVKITARM}/bin/arm-none-eabi-gcc")
set(CMAKE_AR           "${DEVKITARM}/bin/arm-none-eabi-ar")
set(CMAKE_RANLIB       "${DEVKITARM}/bin/arm-none-eabi-ranlib")

# Required because CMake's try_compile defaults to building an executable,
# which fails for a bare-metal/libctru target without the 3dsx linker specs.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# ARM11 MPCore (New/Old 3DS CPU), hard-float ABI, software thread pointer.
# Matches the flags already used by the abandoned fork's Makefile.ctr.
set(CTR_ARCH_FLAGS "-march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft")

set(CMAKE_C_FLAGS_INIT
  "${CTR_ARCH_FLAGS} -mword-relocations -ffunction-sections -fomit-frame-pointer -D__3DS__")
set(CMAKE_CXX_FLAGS_INIT
  "${CMAKE_C_FLAGS_INIT} -fno-rtti")
# NOTE: dosbox-staging's own CMakeLists.txt requests C++23 globally; confirm
# during Fase 1 whether all files actually compiled for the 3DS target need
# that standard, or whether the ctr-specific subset can be pinned lower to
# reduce exposure to libstdc++ features devkitARM's bundled toolchain may
# not fully support. Do not assume compatibility without testing.

set(CMAKE_EXE_LINKER_FLAGS_INIT "-specs=3dsx.specs ${CTR_ARCH_FLAGS}")

# devkitPro portlibs + libctru search paths
set(CMAKE_FIND_ROOT_PATH
  "${DEVKITPRO}/libctru"
  "${DEVKITPRO}/portlibs/3ds"
)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

include_directories(SYSTEM
  "${DEVKITPRO}/libctru/include"
  "${DEVKITPRO}/portlibs/3ds/include"
)
link_directories(
  "${DEVKITPRO}/libctru/lib"
  "${DEVKITPRO}/portlibs/3ds/lib"
)

# Two memory/CPU profiles selected from the CMake command line, consumed by
# the build to pick the right .rsf at packaging time (see platform/ctr/rsf/).
# -DCTR_PROFILE=legacy   -> Old 3DS + compatible New 3DS (80MB, no N3DS clock)
# -DCTR_PROFILE=n3ds      -> New 3DS exclusive (804MHz, 178MB)
if(NOT DEFINED CTR_PROFILE)
  set(CTR_PROFILE "legacy")
endif()
add_compile_definitions(CTR_PROFILE_${CTR_PROFILE}=1)
