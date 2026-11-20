// platform/ctr/ctr_memory.h
//
// 3DS-specific helper to obtain RWX (read/write/execute) memory for the
// CPU dynamic recompiler's code cache. The Horizon kernel does not allow a
// normal userland process to mark memory executable-and-writable at once;
// this module uses the privileged svcBackdoor syscall (declared in the
// title's .rsf under SystemCallAccess, see platform/ctr/rsf/) to patch the
// running process's own SVC access-control bits and gain the extra
// svcControlProcessMemory capability needed.
//
// This ONLY works because:
//   1. the .rsf explicitly lists Backdoor (123) and ControlProcessMemory
//      (112) in SystemCallAccess, and
//   2. the console runs under a CFW (Luma3DS) that does not enforce the
//      normal signature/permission restrictions a retail kernel would put
//      on a title requesting such a dangerous capability.
// It will silently do nothing useful on stock/unpatched firmware, and the
// hardcoded kernel offset in ctrEnableAllServices() is tied to the ARM11
// kernel's internal layout - it has not been verified against every
// Luma3DS/firmware combination. Treat JIT availability as something to be
// checked at runtime (ctr_memory_jit_available), never assumed.
//
// Changes vs. the version in the abandoned MrHuu/dosbox-3ds fork:
//   - _SetMemoryPermission() return value is now meaningful end to end:
//     callers get a real success/failure signal instead of a printf.
//   - _InitializeSvcHack() caches its result so repeated calls (e.g. once
//     per allocated cache region) don't re-run svcBackdoor twice each time.
//   - ctr_memory_jit_available() exposes the cached state so a caller (the
//     dyn_mem_set_access() 3DS branch, see MIGRATION.md) can fall back to
//     core_normal instead of crashing on the first JIT-generated block if
//     the hack failed.
//   - ctr_memory_last_error() returns the raw svcControlProcessMemory
//     result code of the last failed call, for logging.
//
// NOT changed: the kernel-offset hack itself. Nobody on this project has
// re-derived it from scratch; it is inherited as-is from PicoDrive's
// libretro 3DS port. If it ever breaks on a future Luma3DS/kernel
// revision, this file is where to start looking, not somewhere in DOSBox's
// own CPU core.

#ifndef DOSBOX_CTR_MEMORY_H
#define DOSBOX_CTR_MEMORY_H

#ifdef __cplusplus
extern "C" {
#endif

// Attempts to gain the elevated SVC access needed for _SetMemoryPermission
// to succeed. Safe to call more than once; after the first call, further
// calls just return the cached result without re-invoking svcBackdoor.
// Returns 1 on success, 0 on failure.
int _InitializeSvcHack(void);

// Marks a buffer/size region with the given memory permission bits
// (standard 3DS MEMPERM_* values, e.g. 7 = read|write|execute).
// buffer and size MUST already be 0x1000 (page) aligned by the caller -
// this function does not adjust or round them. Passing a size smaller
// than what was actually allocated (as happened in the original fork's
// dyn_cache.h call site, which shaved off the final ~4KB by mistake) will
// leave part of the region without the requested permission; passing a
// size larger than the allocation range is undefined.
// Returns the raw svcControlProcessMemory result code (0 = success).
int _SetMemoryPermission(void *buffer, int size, int permission);

// Returns 1 if _InitializeSvcHack has already succeeded at least once in
// this process, 0 otherwise. Use this to decide whether to even attempt a
// dynamic-recompiler cache allocation, instead of discovering failure only
// after the first generated block is executed.
int ctr_memory_jit_available(void);

// Result code of the last failed _SetMemoryPermission/_InitializeSvcHack
// call, for diagnostic logging. 0 if nothing has failed yet.
int ctr_memory_last_error(void);

#ifdef __cplusplus
}
#endif

#endif // DOSBOX_CTR_MEMORY_H
