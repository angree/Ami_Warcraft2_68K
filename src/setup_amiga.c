/* setup_amiga.c - w2setup, the AmiW2 settings tool (GadTools window on the Workbench screen).
 *
 * One 68k binary for AmigaOS 3.x, AROS 68k and MorphOS (run-time detection only). Edits the keys of
 * w2cfg.h; every other line of the file is kept (w2cfg.c remembers unknown lines).
 *   Save    writes PROGDIR:w2.cfg and deletes ENV:w2.cfg   (permanent)
 *   Use     writes ENV:w2.cfg only                          (until reboot; the game prefers it)
 *   Cancel  writes nothing (also the close gadget and ESC)
 * Choices this machine cannot have are not offered: no AGA/EHB on MorphOS (no chipset), no Paula on
 * MorphOS (its audio.device emulation can hang - !instrukcje 40-MorphOS), no RTG without
 * cybergraphics.library.
 *
 * Test rig (never needed by players): when PROGDIR:w2setup.test exists the tool logs what it found to
 * PROGDIR:w2setup.log, dumps its own window to PROGDIR:w2setup.ppm (read back from the RastPort) and then
 * acts as if the word in that file ("save", "use", default "cancel") had been clicked - so the window can
 * be checked without anyone touching the host's input. */
#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <intuition/gadgetclass.h>
#include <libraries/gadtools.h>
#include <graphics/gfxbase.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/gadtools.h>
#include <cybergraphx/cybergraphics.h>
#include <proto/cybergraphics.h>
#include <stdio.h>
#include <string.h>
#include "w2cfg.h"
#include "version.h"

unsigned long __stack = 65536UL;
struct Library *CyberGfxBase;

static W2Cfg s;                                   /* the settings being edited */
static int g_morphos, g_aga, g_rtg;

static int is_morphos(void) { return FindResident((CONST_STRPTR)"MorphOS") != NULL; }

/* any RTG mode at all (the game decides the exact one at start) */
static int has_rtg(void)
{
	ULONG id = (ULONG)INVALID_ID;
	int n = 0, guard = 0;
	CyberGfxBase = OpenLibrary((CONST_STRPTR)"cybergraphics.library", 41L);
	if (CyberGfxBase == NULL) return 0;
	while ((id = NextDisplayInfo(id)) != (ULONG)INVALID_ID && guard++ < 1000)
		if (IsCyberModeID(id) && ModeNotAvailable(id) == 0) { n++; break; }
	CloseLibrary(CyberGfxBase); CyberGfxBase = NULL;
	return n > 0;
}

/* ---- cycle labels and their value maps (only what this machine can do) ---- */
static const char *lab_disp[W2_BACKEND_COUNT + 1]; static int map_disp[W2_BACKEND_COUNT];
static const char *lab_res[3] = { "640 x 480 (hires)", "320 x 240 (lores)", NULL };
static const char *lab_depth[7] = { "Auto", "8 bit", "15 bit", "16 bit", "24 bit", "32 bit", NULL };
static const int map_depth[6] = { 0, 8, 15, 16, 24, 32 };
static const char *lab_audio[W2_AUDIO_COUNT + 1]; static int map_audio[W2_AUDIO_COUNT];

static int index_of(const int *map, int n, int v)
{
	int i;
	for (i = 0; i < n; i++) if (map[i] == v) return i;
	return 0;
}

static int build_labels(void)
{
	int n = 0;
	lab_disp[n] = "Auto"; map_disp[n++] = W2_BACKEND_AUTO;
	if (g_aga) { lab_disp[n] = "AGA (chipset)"; map_disp[n++] = W2_BACKEND_AGA; }
	if (g_rtg) { lab_disp[n] = "RTG (gfx card)"; map_disp[n++] = W2_BACKEND_RTG; }
	if (!g_morphos) { lab_disp[n] = "EHB (OCS/ECS)"; map_disp[n++] = W2_BACKEND_EHB; }
	lab_disp[n] = "Workbench window"; map_disp[n++] = W2_BACKEND_WB;
	lab_disp[n] = NULL;
	n = 0;
	if (!g_morphos) { lab_audio[n] = "Paula"; map_audio[n++] = W2_AUDIO_PAULA; }
	lab_audio[n] = "AHI"; map_audio[n++] = W2_AUDIO_AHI;
	lab_audio[n] = "Off"; map_audio[n++] = W2_AUDIO_OFF;
	lab_audio[n] = NULL;
	return 0;
}
static int ndisp(void) { int n = 0; while (lab_disp[n]) n++; return n; }
static int naudio(void) { int n = 0; while (lab_audio[n]) n++; return n; }

/* ---- test rig ---- */
static void test_dump(struct Window *w, const char *action)
{
	FILE *f = fopen("PROGDIR:w2setup.ppm", "wb"), *lg = fopen("PROGDIR:w2setup.log", "w");
	struct Screen *sc = w->WScreen;
	int x, y, deep = 0;
	struct Library *cgx = OpenLibrary((CONST_STRPTR)"cybergraphics.library", 41L);
	if (lg) {
		fprintf(lg, W2_NAME " settings %s: %s, AGA %d, RTG %d, window %dx%d, action %s\n", W2_VERSION,
		        g_morphos ? "MorphOS" : "AmigaOS/AROS", g_aga, g_rtg, w->Width, w->Height, action);
		fprintf(lg, "backend=%s resolution=%d rtg_depth=%d audio=%s game_speed=%d logic_skip=%d data_dir=%s\n",
		        w2cfg_backend_name(s.backend), s.resolution, s.rtg_depth, w2cfg_audio_name(s.audio), s.game_speed, s.logic_skip, s.data_dir);
		fclose(lg);
	}
	if (cgx) { CyberGfxBase = cgx; deep = GetCyberMapAttr(sc->RastPort.BitMap, CYBRMATTR_DEPTH) > 8; }
	if (f) {
		fprintf(f, "P6\n%d %d\n255\n", w->Width, w->Height);
		for (y = 0; y < w->Height; y++)
			for (x = 0; x < w->Width; x++) {
				unsigned char c[3];
				if (deep) {
					ULONG v = ReadRGBPixel(w->RPort, (UWORD)x, (UWORD)y);
					c[0] = (unsigned char)(v >> 16); c[1] = (unsigned char)(v >> 8); c[2] = (unsigned char)v;
				} else {
					ULONG t[3];
					LONG pen = ReadPixel(w->RPort, x, y);
					GetRGB32(sc->ViewPort.ColorMap, (ULONG)(pen < 0 ? 0 : pen), 1, t);
					c[0] = (unsigned char)(t[0] >> 24); c[1] = (unsigned char)(t[1] >> 24); c[2] = (unsigned char)(t[2] >> 24);
				}
				fwrite(c, 1, 3, f);
			}
		fclose(f);
	}
	if (cgx) { CloseLibrary(cgx); CyberGfxBase = NULL; }
}

/* ---- the window ---- */
enum { G_DISPLAY = 1, G_RES, G_DEPTH, G_BAR, G_FPS, G_AUDIO, G_MUSIC, G_SFXVOL, G_MUSVOL,
       G_SPEED, G_SKIP, G_SCROLL, G_DATA, G_SAVE, G_USE, G_CANCEL, G_N };
static struct Gadget *g_gad[G_N];

static int textw(struct Screen *scr, const char *t) { return (int)TextLength(&scr->RastPort, (CONST_STRPTR)t, (UWORD)strlen(t)); }

static int maxw(struct Screen *scr, const char *const *t, int n)
{
	int i, m = 0;
	for (i = 0; i < n; i++) { int w = textw(scr, t[i]); if (w > m) m = w; }
	return m;
}

int main(void)
{
	static const char *const lcol[] = { "Display", "Resolution", "RTG depth", "Amiga bar (multitask)", "Show FPS" };
	static const char *const rcol[] = { "Audio", "Music", "Effects volume", "Music volume", "Game speed", "Logic skip", "Scroll speed" };
	struct Screen *scr;
	struct Window *win = NULL;
	struct Gadget *glist = NULL, *g;
	struct NewGadget ng;
	APTR vi = NULL;
	int fh, step, lw1, lw2, gw, x1, x2, y0, y, yr, ww, done = 0, i;
	int action = 0;                                  /* 0 cancel, 1 save, 2 use */
	static char title[64];

	if (GadToolsBase == NULL || ((struct Library *)IntuitionBase)->lib_Version < 37) return 20;
	g_morphos = is_morphos();
	g_aga = !g_morphos && (GfxBase->ChipRevBits0 & GFXF_AA_ALICE) != 0;
	g_rtg = has_rtg();
	w2cfg_load_default(&s);
	if (g_morphos) {                                 /* no chipset, no Paula */
		if (s.backend == W2_BACKEND_AGA || s.backend == W2_BACKEND_EHB) s.backend = W2_BACKEND_AUTO;
		if (s.audio == W2_AUDIO_PAULA) s.audio = W2_AUDIO_AHI;
	}
	if (!g_aga && s.backend == W2_BACKEND_AGA) s.backend = W2_BACKEND_AUTO;
	if (!g_rtg && s.backend == W2_BACKEND_RTG) s.backend = W2_BACKEND_AUTO;
	build_labels();

	if ((scr = LockPubScreen(NULL)) == NULL) return 20;
	if ((vi = GetVisualInfo(scr, TAG_END)) == NULL) { UnlockPubScreen(NULL, scr); return 20; }
	fh = scr->Font ? scr->Font->ta_YSize : 8;
	step = fh + 9;
	lw1 = maxw(scr, lcol, 5) + 12;
	lw2 = maxw(scr, rcol, 7) + 12;
	gw = maxw(scr, lab_disp, ndisp());
	i = maxw(scr, lab_audio, naudio()); if (i > gw) gw = i;
	i = maxw(scr, lab_res, 2); if (i > gw) gw = i;
	i = maxw(scr, lab_depth, 6); if (i > gw) gw = i;
	gw += 32;                                        /* the cycle glyph */
	x1 = scr->WBorLeft + 8 + lw1;
	x2 = x1 + gw + 16 + lw2;
	y0 = y = scr->WBorTop + fh + 1 + 8;

	g = CreateContext(&glist);
	memset(&ng, 0, sizeof ng);
	ng.ng_VisualInfo = vi; ng.ng_TextAttr = scr->Font; ng.ng_Flags = PLACETEXT_LEFT;
	ng.ng_Height = fh + 6;

	/* left column: display */
	ng.ng_LeftEdge = x1; ng.ng_Width = gw;
	ng.ng_TopEdge = y; ng.ng_GadgetText = (UBYTE *)lcol[0]; ng.ng_GadgetID = G_DISPLAY;
	g = g_gad[G_DISPLAY] = CreateGadget(CYCLE_KIND, g, &ng, GTCY_Labels, (ULONG)lab_disp,
	                                    GTCY_Active, (ULONG)index_of(map_disp, ndisp(), s.backend), TAG_END);
	y += step;
	ng.ng_TopEdge = y; ng.ng_GadgetText = (UBYTE *)lcol[1]; ng.ng_GadgetID = G_RES;
	g = g_gad[G_RES] = CreateGadget(CYCLE_KIND, g, &ng, GTCY_Labels, (ULONG)lab_res, GTCY_Active, (ULONG)s.resolution, TAG_END);
	y += step;
	ng.ng_TopEdge = y; ng.ng_GadgetText = (UBYTE *)lcol[2]; ng.ng_GadgetID = G_DEPTH;
	g = g_gad[G_DEPTH] = CreateGadget(CYCLE_KIND, g, &ng, GTCY_Labels, (ULONG)lab_depth,
	                                  GTCY_Active, (ULONG)index_of(map_depth, 6, s.rtg_depth), GA_Disabled, (ULONG)!g_rtg, TAG_END);
	y += step;
	ng.ng_Width = 26;
	ng.ng_TopEdge = y; ng.ng_GadgetText = (UBYTE *)lcol[3]; ng.ng_GadgetID = G_BAR;
	g = g_gad[G_BAR] = CreateGadget(CHECKBOX_KIND, g, &ng, GTCB_Checked, (ULONG)s.titlebar, GTCB_Scaled, TRUE, TAG_END);
	y += step;
	ng.ng_TopEdge = y; ng.ng_GadgetText = (UBYTE *)lcol[4]; ng.ng_GadgetID = G_FPS;
	g = g_gad[G_FPS] = CreateGadget(CHECKBOX_KIND, g, &ng, GTCB_Checked, (ULONG)s.show_fps, GTCB_Scaled, TRUE, TAG_END);

	/* right column: sound and game */
	yr = y0;
	ng.ng_LeftEdge = x2; ng.ng_Width = gw;
	ng.ng_TopEdge = yr; ng.ng_GadgetText = (UBYTE *)rcol[0]; ng.ng_GadgetID = G_AUDIO;
	g = g_gad[G_AUDIO] = CreateGadget(CYCLE_KIND, g, &ng, GTCY_Labels, (ULONG)lab_audio,
	                                  GTCY_Active, (ULONG)index_of(map_audio, naudio(), s.audio), TAG_END);
	yr += step;
	ng.ng_Width = 26;
	ng.ng_TopEdge = yr; ng.ng_GadgetText = (UBYTE *)rcol[1]; ng.ng_GadgetID = G_MUSIC;
	g = g_gad[G_MUSIC] = CreateGadget(CHECKBOX_KIND, g, &ng, GTCB_Checked, (ULONG)s.music, GTCB_Scaled, TRUE, TAG_END);
	yr += step;
	ng.ng_Width = gw - textw(scr, "100") - 8;
	{
		static const int ids[5] = { G_SFXVOL, G_MUSVOL, G_SPEED, G_SKIP, G_SCROLL };
		const int lo[5] = { 0, 0, 1, 0, 1 }, hi[5] = { 100, 100, 5, 8, 5 };
		const int val[5] = { s.sfx_volume, s.music_volume, s.game_speed, s.logic_skip, s.scroll_speed };
		for (i = 0; i < 5; i++) {
			ng.ng_TopEdge = yr; ng.ng_GadgetText = (UBYTE *)rcol[2 + i]; ng.ng_GadgetID = ids[i];
			g = g_gad[ids[i]] = CreateGadget(SLIDER_KIND, g, &ng, GTSL_Min, (ULONG)lo[i], GTSL_Max, (ULONG)hi[i],
			                                 GTSL_Level, (ULONG)val[i], GTSL_LevelFormat, (ULONG)"%3ld", GTSL_MaxLevelLen, 3UL,
			                                 GTSL_LevelPlace, (ULONG)PLACETEXT_RIGHT, GA_RelVerify, TRUE, TAG_END);
			yr += step;
		}
	}
	ww = x2 + gw + 8 + scr->WBorRight;
	y = (y > yr ? y : yr) + 4;

	/* full width: the data folder */
	ng.ng_LeftEdge = x1; ng.ng_Width = x2 + gw - x1; ng.ng_TopEdge = y; ng.ng_Height = fh + 6;
	ng.ng_GadgetText = (UBYTE *)"Data folder"; ng.ng_GadgetID = G_DATA;
	g = g_gad[G_DATA] = CreateGadget(STRING_KIND, g, &ng, GTST_String, (ULONG)s.data_dir, GTST_MaxChars, (ULONG)(sizeof s.data_dir - 1), TAG_END);
	y += step + 6;

	/* Save / Use / Cancel, the Amiga preferences way */
	ng.ng_Flags = PLACETEXT_IN; ng.ng_Width = 90; ng.ng_Height = fh + 8; ng.ng_TopEdge = y;
	ng.ng_LeftEdge = scr->WBorLeft + 8; ng.ng_GadgetText = (UBYTE *)"Save"; ng.ng_GadgetID = G_SAVE;
	g = g_gad[G_SAVE] = CreateGadget(BUTTON_KIND, g, &ng, TAG_END);
	ng.ng_LeftEdge = (ww - 90) / 2; ng.ng_GadgetText = (UBYTE *)"Use"; ng.ng_GadgetID = G_USE;
	g = g_gad[G_USE] = CreateGadget(BUTTON_KIND, g, &ng, TAG_END);
	ng.ng_LeftEdge = ww - scr->WBorRight - 8 - 90; ng.ng_GadgetText = (UBYTE *)"Cancel"; ng.ng_GadgetID = G_CANCEL;
	g = g_gad[G_CANCEL] = CreateGadget(BUTTON_KIND, g, &ng, TAG_END);
	y += ng.ng_Height + 8;

	if (g != NULL) {
		snprintf(title, sizeof title, W2_NAME " settings %s", W2_VERSION);
		win = OpenWindowTags(NULL,
		                     WA_Title, (ULONG)title, WA_PubScreen, (ULONG)scr,
		                     WA_Left, 20UL, WA_Top, (ULONG)(scr->BarHeight + 4),
		                     WA_Width, (ULONG)ww, WA_Height, (ULONG)(y + scr->WBorBottom),
		                     WA_Gadgets, (ULONG)glist, WA_DragBar, TRUE, WA_DepthGadget, TRUE, WA_CloseGadget, TRUE,
		                     WA_Activate, TRUE, WA_SimpleRefresh, TRUE,
		                     WA_IDCMP, (ULONG)(IDCMP_CLOSEWINDOW | IDCMP_REFRESHWINDOW | IDCMP_VANILLAKEY |
		                                       CYCLEIDCMP | CHECKBOXIDCMP | SLIDERIDCMP | STRINGIDCMP | BUTTONIDCMP),
		                     TAG_END);
	}
	UnlockPubScreen(NULL, scr);
	if (win == NULL) { FreeGadgets(glist); FreeVisualInfo(vi); return 20; }
	GT_RefreshWindow(win, NULL);

	{
		FILE *t = fopen("PROGDIR:w2setup.test", "r");
		if (t) {
			char w4[16] = "";
			if (!fgets(w4, sizeof w4, t)) w4[0] = 0;
			fclose(t);
			action = !strncmp(w4, "save", 4) ? 1 : !strncmp(w4, "use", 3) ? 2 : 0;
			Delay(50);
			test_dump(win, action == 1 ? "save" : action == 2 ? "use" : "cancel");
			done = 1;
		}
	}

	while (!done) {
		struct IntuiMessage *m;
		WaitPort(win->UserPort);
		while (!done && (m = GT_GetIMsg(win->UserPort)) != NULL) {
			ULONG cls = m->Class;
			UWORD code = m->Code;
			struct Gadget *gg = (struct Gadget *)m->IAddress;
			GT_ReplyIMsg(m);
			if (cls == IDCMP_CLOSEWINDOW) done = 1;
			else if (cls == IDCMP_REFRESHWINDOW) { GT_BeginRefresh(win); GT_EndRefresh(win, TRUE); }
			else if (cls == IDCMP_VANILLAKEY && code == 27) done = 1;
			else if (cls == IDCMP_GADGETUP || cls == IDCMP_GADGETDOWN || cls == IDCMP_MOUSEMOVE) {
				switch (gg->GadgetID) {
				case G_DISPLAY: if (code < ndisp()) s.backend = map_disp[code]; break;
				case G_RES:     s.resolution = code ? W2_RES_LORES : W2_RES_HIRES; break;
				case G_DEPTH:   if (code < 6) s.rtg_depth = map_depth[code]; break;
				case G_BAR:     s.titlebar = (gg->Flags & GFLG_SELECTED) != 0; break;
				case G_FPS:     s.show_fps = (gg->Flags & GFLG_SELECTED) != 0; break;
				case G_AUDIO:   if (code < naudio()) s.audio = map_audio[code]; break;
				case G_MUSIC:   s.music = (gg->Flags & GFLG_SELECTED) != 0; break;
				case G_SFXVOL:  s.sfx_volume = (WORD)code; break;
				case G_MUSVOL:  s.music_volume = (WORD)code; break;
				case G_SPEED:   s.game_speed = (WORD)code; break;
				case G_SKIP:    s.logic_skip = (WORD)code; break;
				case G_SCROLL:  s.scroll_speed = (WORD)code; break;
				case G_SAVE:    action = 1; done = 1; break;
				case G_USE:     action = 2; done = 1; break;
				case G_CANCEL:  action = 0; done = 1; break;
				default: break;
				}
			}
		}
	}
	if (action) {                                    /* the string gadget's text as it stands now */
		struct StringInfo *si = (struct StringInfo *)g_gad[G_DATA]->SpecialInfo;
		if (si && si->Buffer && si->Buffer[0]) snprintf(s.data_dir, sizeof s.data_dir, "%s", (const char *)si->Buffer);
	}
	CloseWindow(win);
	FreeGadgets(glist);
	FreeVisualInfo(vi);
	if (action == 1) {
		if (w2cfg_save(&s, W2CFG_FILE) != 0) return 10;
		DeleteFile((CONST_STRPTR)W2CFG_ENV_FILE);
	} else if (action == 2) {
		if (w2cfg_save(&s, W2CFG_ENV_FILE) != 0) return 10;
	}
	return 0;
}
