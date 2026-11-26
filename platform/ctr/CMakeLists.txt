# platform/ctr/CMakeLists.txt
#
# Standalone CMake project for the Nintendo 3DS target. Deliberately its
# OWN top-level project - it does NOT include, add_subdirectory(), or in
# any other way invoke dosbox-staging's own root CMakeLists.txt. That file
# assumes a hosted OS (WIN32/APPLE/LINUX) end to end and resolves its
# dependencies (SDL3, FluidSynth, mt32emu, PNG, OpenGL, iir...) through
# vcpkg - none of which exists for devkitARM/Horizon OS. Running the 3DS
# build through it, even after patching the CPU-detection block, leads to
# a long chain of REQUIRED find_package() failures (confirmed by reading
# ahead in that file - see MIGRATION.md, "Fase 2c" for the list found).
#
# Invoke this directly, pointed at this directory, not at the repo root:
#   cmake -S platform/ctr -B build-smoketest \
#         -DCMAKE_TOOLCHAIN_FILE=../../toolchain/devkitarm-3ds.cmake
#   cmake --build build-smoketest
#
# Current scope: ONLY the toolchain smoke test (hello/main.c - a trivial
# libctru console program with zero dosbox-staging dependencies). This is
# intentional and temporary: before trusting any result from compiling
# dosbox-staging's own source against this toolchain, we want independent
# proof that devkitARM + CMake + this CI pipeline can produce a .3dsx that
# actually boots on real hardware. Once that's confirmed on the New3DS XL,
# this file is where the real dosbox-staging source subset (cpu/, dos/,
# hardware/, ints/, misc/, shell/, fpu/ - minus anything needing
# SDL3/FluidSynth/mt32emu) gets added as a second target, alongside the
# platform/ctr glue already in this directory (ctr_memory.c, ctr_cache.c,
# and eventually the rest of the ctr_* platform layer from Fase 1).
#
# NOT yet done, deliberately out of scope for this file as it stands:
# picking that source subset. Guessing it without a compiler to check
# against is how we got into patching the wrong CMakeLists.txt in the
# first place - it should be done incrementally, file by file, against
# real build errors once this smoke test confirms the pipeline works.

cmake_minimum_required(VERSION 3.25)
project(dosbox3ds_toolchain_smoketest LANGUAGES C)

if(NOT DEFINED ENV{DEVKITPRO})
  message(FATAL_ERROR "DEVKITPRO is not set. export DEVKITPRO=<path to>devkitpro")
endif()

add_executable(hello hello/main.c)
target_link_libraries(hello PRIVATE ctru m)

find_program(SMDHTOOL smdhtool PATHS "$ENV{DEVKITPRO}/tools/bin")
find_program(TDSXTOOL 3dsxtool PATHS "$ENV{DEVKITPRO}/tools/bin")
if(NOT SMDHTOOL)
  message(FATAL_ERROR "smdhtool not found - is $DEVKITPRO/tools/bin on PATH?")
endif()
if(NOT TDSXTOOL)
  message(FATAL_ERROR "3dsxtool not found - is $DEVKITPRO/tools/bin on PATH?")
endif()

# libctru ships a generic placeholder icon at this path - using it here
# rather than shipping our own PNG, since the icon's content doesn't
# matter at all for what this smoke test is checking.
set(CTR_DEFAULT_ICON "$ENV{DEVKITPRO}/libctru/default_icon.png")

add_custom_command(TARGET hello POST_BUILD
  COMMAND "${SMDHTOOL}" --create
          "DOSBox-3DS toolchain test"
          "Fase 0 pipeline smoke test, not the emulator"
          "dosbox-3ds migration"
          "${CTR_DEFAULT_ICON}"
          "$<TARGET_FILE_DIR:hello>/hello.smdh"
  COMMAND "${TDSXTOOL}"
          "$<TARGET_FILE:hello>"
          "$<TARGET_FILE_DIR:hello>/hello.3dsx"
          "--smdh=$<TARGET_FILE_DIR:hello>/hello.smdh"
  BYPRODUCTS
    "${CMAKE_CURRENT_BINARY_DIR}/hello.smdh"
    "${CMAKE_CURRENT_BINARY_DIR}/hello.3dsx"
  COMMENT "Packaging hello.3dsx"
)