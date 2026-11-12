/* w2_sys.c - the few OS services AmiW2 needs beyond amiga_gfx (plain C; Amiga headers only here). */
#include <exec/types.h>
#include <exec/execbase.h>
#include <graphics/gfxbase.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <dos/dos.h>
#include "w2_sys.h"

extern struct ExecBase *SysBase;

void w2_sleep_ms(int ms)
{
	LONG ticks = ms / 20;
	Delay(ticks > 0 ? ticks : 1);
}

void w2_wait_tof(void)
{
	WaitTOF();
}

void w2_mkdir(const char *path)
{
	BPTR l = Lock((CONST_STRPTR)path, ACCESS_READ);
	if (l) { UnLock(l); return; }
	l = CreateDir((CONST_STRPTR)path);
	if (l) UnLock(l);
}

int w2_file_exists(const char *path)
{
	BPTR l = Lock((CONST_STRPTR)path, ACCESS_READ);
	if (l) { UnLock(l); return 1; }
	return 0;
}

int w2_delete(const char *path)
{
	return DeleteFile((CONST_STRPTR)path) ? 1 : 0;
}

int w2_has_aga(void)
{
	if (FindResident((CONST_STRPTR)"MorphOS") != NULL) return 0;   /* MorphOS has no chipset at all */
	return (GfxBase->ChipRevBits0 & GFXF_AA_ALICE) != 0;
}

int w2_cpu(void)
{
	UWORD a = SysBase->AttnFlags;
	if (a & 0x80) return 60;          /* AFF_68060 (not in every NDK) */
	if (a & AFF_68040) return 40;
	if (a & AFF_68030) return 30;
	if (a & AFF_68020) return 20;
	if (a & AFF_68010) return 10;
	return 0;
}

unsigned long w2_kickstart(void)
{
	return ((unsigned long)SysBase->LibNode.lib_Version << 16) | SysBase->LibNode.lib_Revision;
}

int w2_dir_list(const char *path, void (*fn)(const char *name, void *ud), void *ud)
{
	struct FileInfoBlock *fib;
	BPTR l;
	int n = 0;
	if (!fn) return -1;
	l = Lock((CONST_STRPTR)path, ACCESS_READ);
	if (!l) return -1;
	fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL);
	if (!fib) { UnLock(l); return -1; }
	if (Examine(l, fib) && fib->fib_DirEntryType > 0) {
		while (ExNext(l, fib)) {
			if (fib->fib_DirEntryType < 0) { fn((const char *)fib->fib_FileName, ud); n++; }
		}
	}
	FreeDosObject(DOS_FIB, fib);
	UnLock(l);
	return n;
}
