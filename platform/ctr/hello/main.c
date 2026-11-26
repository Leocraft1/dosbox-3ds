// platform/ctr/hello/main.c
//
// Deliberately NOT part of dosbox-staging. This is a standalone,
// textbook-standard libctru console program with no dependency on
// anything in src/ - its only job is to prove that the devkitARM +
// CMake + CI pipeline (toolchain/devkitarm-3ds.cmake,
// platform/ctr/CMakeLists.txt, .github/workflows/build-3ds.yml) can
// actually produce a working .3dsx end to end, before adding the much
// larger risk of compiling dosbox-staging's own source against this
// toolchain. If this doesn't build or doesn't run on real hardware, the
// problem is in the toolchain/CI, not in anything dosbox-specific - that
// distinction is the entire reason this file exists.

#include <3ds.h>
#include <stdio.h>

int main(void)
{
	gfxInitDefault();
	consoleInit(GFX_TOP, NULL);

	printf("dosbox-3ds staging-port\n");
	printf("toolchain smoke test OK\n");
	printf("\nPress START to exit.\n");

	while (aptMainLoop()) {
		hidScanInput();
		u32 kDown = hidKeysDown();
		if (kDown & KEY_START) {
			break;
		}

		gfxFlushBuffers();
		gfxSwapBuffers();
		gspWaitForVBlank();
	}

	gfxExit();
	return 0;
}
