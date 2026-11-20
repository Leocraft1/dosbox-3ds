// platform/ctr/ctr_cache.c
//
// See ctr_cache.h for the full rationale, including why this is NOT just
// a straightforward call to the stock svcFlushProcessDataCache (it only
// covers the data cache - see the corrected explanation there).

#include <3ds.h>

#include "ctr_cache.h"

// svcInvalidateEntireInstructionCache (SVC 0x94) is a Luma3DS k11_extension
// addition, not a stock Horizon syscall - it has no wrapper in stock
// libctru, so provide our own, following the same minimal-wrapper shape
// Luma3DS's own csvc.s uses for its custom SVCs. Declared static inline
// here rather than pulled from Luma3DS's csvc.h directly, to avoid taking
// a source dependency on the Luma3DS repository for one instruction.
//
// NOTE: void return (no Result code) matches Luma3DS's own declaration in
// sysmodules/rosalina/include/csvc.h - this call cannot report failure to
// the caller. If it's unavailable (non-Luma CFW, or a future Luma release
// that renumbers/removes it), this will most likely either no-op against
// an unmapped SVC slot or crash outright, not fail gracefully. That's a
// real limitation of using an undocumented-to-homebrew custom SVC; it's
// accepted here because the target is explicitly Luma3DS and because the
// alternative (no icache invalidation at all) is strictly worse.
static inline void svcInvalidateEntireInstructionCache(void)
{
	__asm__ volatile("svc 0x94" ::: "r0", "r1", "r2", "r3", "r12", "lr", "cc", "memory");
}

int ctr_cache_sync(void *addr, unsigned int size)
{
	Result res = svcFlushProcessDataCache(CUR_PROCESS_HANDLE, addr, size);

	// Always invalidate the instruction cache even if the data-cache
	// flush above failed: a failed flush still may have written SOME
	// cache lines back before erroring, and the data written to the
	// cache block by the JIT's normal store instructions may have
	// already reached memory through ordinary cache eviction
	// regardless of this SVC's result. Skipping the icache step on a
	// flush failure would only make a bad situation strictly worse by
	// also leaving stale fetched instructions in place.
	svcInvalidateEntireInstructionCache();

	return (int)res;
}
