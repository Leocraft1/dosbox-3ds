// platform/ctr/ctr_memory.c
//
// See ctr_memory.h for the full explanation of what this does and why.
// Core mechanism taken from PicoDrive's libretro 3DS port, as in the
// original fork; this version adds error propagation, caching and a
// queryable availability flag that the original lacked.

#include <3ds.h>
#include <stdio.h>

#include "ctr_memory.h"

typedef int (*ctr_callback_type)(void);

static int hack3dsSvcInitialized = 0; // 0 = not attempted, 1 = ok, -1 = failed
static int ctr_memory_errno = 0;

static unsigned int s0, s1, s2, s3;

//-----------------------------------------------------------------------------
// Internal function that enables all services. This must be run via
// svcBackdoor (i.e. with kernel privileges), which is only reachable at all
// because the title's .rsf declares Backdoor in SystemCallAccess and the
// console runs under a CFW that does not reject that request.
//
// The 0xFFFF9000 dereference below walks a fixed, kernel-version-specific
// offset to reach the current KProcess's SVC access-control bitmask. It is
// NOT derived from any public, versioned kernel header - it was reverse
// engineered for a specific (undocumented, in this codebase) kernel build.
// If this ever starts corrupting memory or hanging the console after a
// system/Luma3DS update, this is the function to suspect first.
//-----------------------------------------------------------------------------
static void ctrEnableAllServices(void)
{
	__asm__ volatile("cpsid aif");

	unsigned int *svc_access_control = *(*(unsigned int***)0xFFFF9000 + 0x22) - 0x6;

	s0 = svc_access_control[0];
	s1 = svc_access_control[1];
	s2 = svc_access_control[2];
	s3 = svc_access_control[3];

	svc_access_control[0] = 0xFFFFFFFE;
	svc_access_control[1] = 0xFFFFFFFF;
	svc_access_control[2] = 0xFFFFFFFF;
	svc_access_control[3] = 0x3FFFFFFF;
}

int _SetMemoryPermission(void *buffer, int size, int permission)
{
	Handle currentHandle;
	Result dup = svcDuplicateHandle(&currentHandle, 0xFFFF8001);
	if (dup != 0) {
		ctr_memory_errno = dup;
		return dup;
	}

	int res = svcControlProcessMemory(currentHandle, buffer, 0, size,
	                                   MEMOP_PROT, permission);
	svcCloseHandle(currentHandle);

	if (res != 0) {
		ctr_memory_errno = res;
	}
	return res;
}

int _InitializeSvcHack(void)
{
	// Cached: don't re-run svcBackdoor (and re-toggle the global access
	// control bits) once we already know the outcome. The original fork
	// re-ran this unconditionally from every dyn_cache.h call site.
	if (hack3dsSvcInitialized != 0) {
		return hack3dsSvcInitialized > 0;
	}

	svcBackdoor((ctr_callback_type)ctrEnableAllServices);
	svcBackdoor((ctr_callback_type)ctrEnableAllServices);

	if (s0 != 0xFFFFFFFE || s1 != 0xFFFFFFFF || s2 != 0xFFFFFFFF || s3 != 0x3FFFFFFF) {
		fprintf(stderr,
		        "ctr_memory: svcBackdoor hack failed (got %08x %08x %08x %08x). "
		        "Dynamic recompiler cache will not be executable; "
		        "fall back to core_normal/core_simple.\n",
		        s0, s1, s2, s3);
		hack3dsSvcInitialized = -1;
		ctr_memory_errno = -1;
		return 0;
	}

	hack3dsSvcInitialized = 1;
	return 1;
}

int ctr_memory_jit_available(void)
{
	return hack3dsSvcInitialized > 0;
}

int ctr_memory_last_error(void)
{
	return ctr_memory_errno;
}
