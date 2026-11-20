// platform/ctr/ctr_cache.h
//
// Replaces the original fork's platform/ctr/ctr_cache.s. History of this
// file within this migration (kept deliberately, don't delete - it matters
// for anyone revisiting this code):
//
//   Draft 1: proposed replacing the original's custom-SVC CP15 hack with
//   the stock kernel's svcFlushProcessDataCache/svcInvalidateProcessData-
//   Cache alone. WRONG - and confirmed wrong by multiple independent,
//   recent sources (a real New3DS XL hardware crash dump analysed in
//   psx3ds v0.2.3's release notes; an Azahar emulator PR from Oct 2026;
//   a Luma3DS GitHub issue): those two stock SVCs only clean/invalidate
//   the L1 DATA cache. They do not touch the L1 INSTRUCTION cache or the
//   branch target buffer (BTB) at all. Using them alone would leave the
//   CPU free to keep fetching and executing stale, pre-overwrite
//   instructions from the icache - exactly the bug class the original
//   fork's CP15 hack existed to prevent, silently reintroduced.
//
//   Draft 2 (this one): data cache half via the stock
//   svcFlushProcessDataCache (SVC 0x54, 3dbrew.org numbering - not
//   switchbrew.org's Switch numbering, which an earlier pass of this
//   research mixed up). Instruction cache half via Luma3DS's own CUSTOM
//   svcInvalidateEntireInstructionCache (SVC 0x94, from Luma3DS's
//   sysmodules/rosalina/include/csvc.h). This is the same idiom used by
//   several actively maintained 3DS JIT projects (exsilium/3ds-d2,
//   codescv/psx3ds) as of 2026.
//
// Why the WHOLE-cache invalidate (0x94) and not the range-specific one
// (0x93 svcInvalidateInstructionCacheRange): exsilium/3ds-d2's own commit
// history documents 0x93 faulting in the kernel specifically when the
// target range is RWX (exactly our case - the JIT cache). 0x94 costs a
// little more per call but is the one actually reported working against
// RWX pages. Don't "optimize" this back to the range variant without
// testing on real hardware first - the available evidence says it faults
// for exactly this use case.
//
// These are Luma3DS kernel-extension SVCs, not stock Horizon ones: they
// only exist because Luma3DS's k11_extension hooks unused SVC table slots
// to add them. They will not work (and in fact won't exist at all) on
// stock firmware without Luma3DS, or on a CFW that doesn't carry an
// equivalent k11_extension. This is fine for this project (the target is
// explicitly Luma3DS), but don't assume csvc.h-style calls are portable
// to any future non-Luma CFW without checking that CFW's own extensions.
//
// OPEN QUESTION, worth settling empirically before relying on this in
// Fase 2: Luma3DS's "Standard features" wiki states that, since Luma3DS
// v9.0, "SVC, service, archive, and ARM9 exheader permission checks are
// patched out by default" - if that covers the exheader's SystemCallAccess
// bitmap unconditionally, then platform/ctr/src/ctr_memory.c's whole
// svcBackdoor escalation (and possibly declaring Backdoor/
// ControlProcessMemory in the .rsf at all) may no longer be necessary on
// any current Luma3DS install, and svcControlProcessMemory(MEMOP_PROT)
// might simply work unescalated. This hasn't been tested here - do that
// test early in Fase 2 bring-up, since a positive result would let
// ctr_memory.c be deleted outright instead of merely corrected.

#ifndef DOSBOX_CTR_CACHE_H
#define DOSBOX_CTR_CACHE_H

#ifdef __cplusplus
extern "C" {
#endif

// Writes back the data cache for [addr, addr+size) via the stock
// svcFlushProcessDataCache, then invalidates the ENTIRE instruction cache
// (and BTB) via Luma3DS's svcInvalidateEntireInstructionCache - not just
// the given range, see rationale above. Call once per finished JIT cache
// block (matches cache_block_closing()'s granularity), not per instruction;
// the whole-icache cost is amortized the same way the original CP15 hack's
// whole-cache clean/invalidate already was.
//
// addr/size are taken for the data-cache half only; kept in the signature
// (rather than reverting to the original's no-argument whole-cache call)
// because a range-limited data cache flush is strictly better once we're
// paying for a cheap stock SVC per block anyway - no reason to flush more
// than was actually written.
//
// Returns 0 on success, or the first non-zero SVC result code encountered.
int ctr_cache_sync(void *addr, unsigned int size);

#ifdef __cplusplus
}
#endif

#endif // DOSBOX_CTR_CACHE_H
