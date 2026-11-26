// SPDX-FileCopyrightText:  2020-2025 The DOSBox Staging Team
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef DOSBOX_DOSBOX_CONFIG_H
#define DOSBOX_DOSBOX_CONFIG_H

// Current DOSBox Staging version without 'v' prefix (e.g., 0.79.1,
// 0.81.1-alpha)
#define DOSBOX_VERSION "${DOSBOX_VERSION}"

#define DOSBOX_VERSION_SHORT "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"

// Minimum 5-char long Git hash of the build; can be longer to guarantee
// uniqueness (e.g., da3c5, c22ef8)
#define BUILD_GIT_HASH "${BUILD_GIT_HASH}"


// Operating System
//

// Defined if compiling for OS from BSD family
#cmakedefine BSD

// Defined if compiling for Linux (non-Android)
#cmakedefine LINUX

// Defined if compiling for macOS
#cmakedefine MACOSX

// Defined if compiling for Windows (any option)
#ifndef WIN32
#cmakedefine WIN32
#endif


// CPU and FPU emulation
//
// These defines are mostly relevant to modules src/cpu/ and src/fpu/

// Target CPU we're compiling for; only one of these can be 1
#cmakedefine01 C_TARGET_CPU_ARM
#cmakedefine01 C_TARGET_CPU_X86

// 32-bit ARM (e.g. ARMv6K/ARM11 on the Nintendo 3DS) - distinct from
// C_TARGET_CPU_ARM, which in this codebase means AArch64 only (see
// src/cpu/core_dynrec.cpp, which only pairs C_TARGET_CPU_ARM with the
// AArch64-only core_dynrec/risc_armv8le.h backend). Not part of
// dosbox-staging's own supported-architecture list; see MIGRATION.md.
#cmakedefine01 C_TARGET_CPU_ARM32

#if C_TARGET_CPU_ARM32
// risc_armv4le.h (ported from DOSBox-X) still uses the older,
// DOSBox-classic numeric C_TARGETCPU enum internally to choose between
// its -o3.h/-thumb*.h variants, rather than the boolean C_TARGET_CPU_ARM32
// above. Both symbols below MUST be defined as distinct integers - if
// either is left to the preprocessor's implicit 0 for an undefined
// identifier, "#if C_TARGETCPU == ARMV7LE" silently becomes "0 == 0" and
// wrongly selects the ARMv7 code path on this ARMv6K target. See
// GLUE_CHECKLIST.md for how this was found (DOSBox-X deliberately
// excludes ARMV6LE from that optimisation).
#define ARMV7LE 1
#define C_TARGETCPU 0
#endif

// Define to 1 if target CPU supports unaligned memory access
#cmakedefine01 C_UNALIGNED_MEMORY

// Define to 1 if the target platform needs per-page dynamic core write or execute (W^X) tagging
#cmakedefine01 C_PER_PAGE_W_OR_X

// Define to 1 to use the x86_64 dynamic CPU core
// Can not be used together with C_DYNREC
#cmakedefine01 C_DYNAMIC_X86

// Define to 1 to use the recompiling CPU core
// Can not be used together with C_DYNAMIC_X86
#cmakedefine01 C_DYNREC

// Define to 1 to use the FPU core implemented in x86 assembler
#cmakedefine01 C_FPU_X86

// Define to 1 to use inlined memory functions in CPU core
#define C_CORE_INLINE 1


// Emulator features
//
// Turn on or off optional emulator features that depend on external libraries.
// This way it's easier to port or package on a new platform, where these
// libraries might be missing.

// Define to 1 to use opengl display output support
#cmakedefine01 C_OPENGL

// Define to 1 to enable internal debugger (using ncurses or pdcurses)
#cmakedefine01 C_DEBUGGER

// Define to 1 to enable heavy debugging (requires C_DEBUGGER)
#cmakedefine01 C_HEAVY_DEBUGGER

// Define to 1 to enable MT-32 emulator
#cmakedefine01 C_MT32EMU

// Define to 1 to enable mouse mapping support
#cmakedefine01 C_MANYMOUSE

// ManyMouse optionally supports the X Input 2.0 protocol (regardless of OS). It
// uses the following define to definitively indicate if it should or shouldn't
// use the X Input 2.0 protocol. If this is left undefined, then ManyMouse makes
// an assumption about availability based on OS type.
#cmakedefine01 SUPPORT_XINPUT2

// Define to 1 to enable ALSA MIDI support
#cmakedefine01 C_ALSA


// Defines for checking availability of standard functions and structs.
//
// Sometimes available functions, structs, or struct fields differ slightly
// between operating systems.

// Define to 1 when using the native zlib-ng API
#cmakedefine01 C_ZLIB_NG

// Defined if synchronous I/O multiplexing is available
#cmakedefine HAVE_FD_ZERO

// Defined if function clock_gettime is available
#cmakedefine HAVE_CLOCK_GETTIME

// Defined if function __builtin_available is available
#cmakedefine HAVE_BUILTIN_AVAILABLE

// Defined if function __builtin___clear_cache is available
#cmakedefine HAVE_BUILTIN_CLEAR_CACHE

// Defined if function mprotect is available
#cmakedefine HAVE_MPROTECT

// Defined if function mmap is available
#cmakedefine HAVE_MMAP

// Defined if mmap flag MAPJIT is available
#cmakedefine HAVE_MAP_JIT

// Defined if function pthread_jit_write_protect_np is available
#cmakedefine HAVE_PTHREAD_WRITE_PROTECT_NP

// Defined if function sys_icache_invalidate is available
#cmakedefine HAVE_SYS_ICACHE_INVALIDATE

// Defined if function pthread_setname_np is available
#cmakedefine HAVE_PTHREAD_SETNAME_NP

// Defind if function setpriority is available
#cmakedefine HAVE_SETPRIORITY

// Defind if function strnlen is available
#cmakedefine HAVE_STRNLEN

// field d_type in struct dirent is not defined in POSIX
// Some OSes do not implement it (e.g. Haiku)
#cmakedefine HAVE_STRUCT_DIRENT_D_TYPE


// Available headers
//
// Checks for non-POSIX headers and POSIX headers not supported on Windows.

#cmakedefine HAVE_LIBGEN_H
#cmakedefine HAVE_NETINET_IN_H
#cmakedefine HAVE_STDLIB_H
#cmakedefine HAVE_STRINGS_H
#cmakedefine HAVE_SYS_SOCKET_H
#cmakedefine HAVE_SYS_TYPES_H
#cmakedefine HAVE_SYS_XATTR_H


// Windows-related defines
//

// Prevent <windows.h> from clobbering std::min and std::max
#ifndef NOMINMAX
#cmakedefine NOMINMAX 1
#endif

// Enables mathematical constants (such as M_PI) in Windows math.h header
// https://docs.microsoft.com/en-us/cpp/c-runtime-library/math-constants
#cmakedefine _USE_MATH_DEFINES

// Modern MSVC provides POSIX-like routines, so prefer that over built-in
#cmakedefine HAVE_STRNLEN

// Holds the CMAKE_INSTALL_DATADIR specified during project setup. This can
// be used as a fallback if the user hasn't populated their
// XDG_DATA_HOME or XDG_DATA_DIRS.
#cmakedefine CUSTOM_DATADIR "@CUSTOM_DATADIR@"


#endif // DOSBOX_DOSBOX_CONFIG_H
