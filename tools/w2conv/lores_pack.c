/* lores_pack.c - w2conv step: the lores art packs <data>/lores-{summer,winter,wasteland}.pack and the
 * ECS Extra-Half-Brite art packs <data>/ehb-{summer,winter,wasteland}.pack (the same halving, quantised
 * to the 64-colour EHB palette of src/ehb.c - see docs/CONVERTER.md).
 *
 * The game's 320x240 mode builds these itself on its first lores start (minutes on a 68030). w2conv
 * builds them once on the PC instead, with the GAME'S OWN CODE: src/lores.c (+ assets.c, war.c,
 * w2gfx_decode.c, w2cfg.c, w2_tables.c, font35.c) linked unchanged, so the packs are byte-identical to
 * the ones the game writes. The packs are endian-neutral (little-endian fields, byte pixels).
 * This file is only the shim those modules need from the platform layer (w2_log, amigagfx_*).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#ifdef _WIN32
#include <direct.h>
#define CHDIR _chdir
#define GETCWD _getcwd
#else
#include <unistd.h>
#define CHDIR chdir
#define GETCWD getcwd
#endif
#include "w2cfg.h"
#include "assets.h"
#include "lores.h"
#include "lores_pack.h"

int lores_prepare(void);   /* src/lores.c (declared for assets.c in assets.h on the game side) */

/* ---- platform shim ---- */
static int g_quiet_log;

void w2_log(const char *fmt, ...)
{
	va_list ap;
	if (g_quiet_log) return;
	va_start(ap, fmt);
	printf("  ");
	vprintf(fmt, ap);
	printf("\n");
	va_end(ap);
	fflush(stdout);
}

unsigned long amigagfx_millis(void) { return (unsigned long)((double)clock() * 1000.0 / CLOCKS_PER_SEC); }
void amigagfx_set_palette(const unsigned char *rgb, int first, int count) { (void)rgb; (void)first; (void)count; }
void amigagfx_set_ehb_palette(const unsigned char *rgb64) { (void)rgb64; }
void amigagfx_pointer_colours(const unsigned char *rgb9) { (void)rgb9; }

/* ---- progress ---- */
static const char *const k_era[3] = { "summer", "winter", "wasteland" };

static const char *g_kind = "lores";

static void on_progress(int era, int done, int total)
{
	static int last = -1;
	int pc = total > 0 ? done * 100 / total : 100;
	if (done == 0) last = -1;
	if (pc / 5 == last) return;
	last = pc / 5;
	printf("\r  %s-%-9s %3d%%", g_kind, era >= 0 && era < 3 ? k_era[era] : "?", pc);
	if (done >= total) printf("\n");
	fflush(stdout);
}

int lores_packs_build(const char *outdir, int ehb)
{
	char cwd[1024];
	int r;
	if (!GETCWD(cwd, sizeof cwd)) cwd[0] = 0;
	/* Work from the folder that holds "data": the game modules keep paths in 128/300-byte buffers,
	 * and a player's Windows path may be long. The packs do not depend on where they are built. */
	if (CHDIR(outdir)) { fprintf(stderr, "w2conv: cannot enter %s\n", outdir); return -1; }
	w2cfg_defaults(&w2_cfg);
	w2_cfg.resolution = W2_RES_LORES;
	snprintf(w2_cfg.data_dir, sizeof w2_cfg.data_dir, "data");
	assets_set_ehb(ehb);                  /* the game's own switch: EHB palette + ehb-<era>.pack */
	g_kind = ehb ? "ehb" : "lores";
	g_quiet_log = 1;                      /* "assets: ... ok" lines are noise here */
	r = assets_init();
	g_quiet_log = 0;
	if (r) { fprintf(stderr, "w2conv: lores packs: the archives in %s/data cannot be read\n", outdir); if (cwd[0] && CHDIR(cwd)) cwd[0] = 0; return -1; }
	lores_set_progress(on_progress);
	r = lores_prepare();
	assets_shutdown();
	assets_set_ehb(0);
	if (cwd[0] && CHDIR(cwd)) cwd[0] = 0;
	return r;
}
