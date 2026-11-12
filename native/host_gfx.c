/* host_gfx.c - the amigagfx_* API (amiga_gfx.h) on a Linux/WSL host, for fast testing without an
 * emulator. Built only by build/build-host.sh (-DW2_HOST); never part of the Amiga build.
 *
 *  - display: an in-memory 8-bit chunky buffer of the requested size (exact allocation, so ASan
 *    catches any write past the game area) plus a separate DISPLAY copy that only amigagfx_blit
 *    updates - a shot shows what the Amiga would show, so a missing blit is visible in the PNG.
 *  - clock: VIRTUAL by default (deterministic): an empty amigagfx_poll advances it W2_POLL_MS (1 ms),
 *    every amigagfx_millis call W2_MILLIS_US (20 us, so busy-waits terminate), host sleeps by their
 *    length. W2_REALTIME=1 uses the real monotonic clock instead.
 *  - input: a script file (W2_INPUT), see docs/HOST_TEST.md. Optional SDL2 window (-DHOST_SDL, needs
 *    DISPLAY and no W2_HEADLESS).
 *  - shots: PNG (own stored-deflate writer, no zlib) into W2_SHOTDIR.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include "amiga_gfx.h"

#ifdef HOST_SDL
#include <SDL2/SDL.h>
#endif

/* ---- shared with host_sys.c / host_audio.c / tools/host_selftest.c ------------------------ */
unsigned long host_clock_us(void);
void host_clock_advance_ms(unsigned long ms);
int  host_realtime(void);
int  host_shot(const char *name);
const char *host_shotdir(void);
void host_logf(const char *fmt, ...);

/* ---- state -------------------------------------------------------------------------------- */
static unsigned char *g_chunky, *g_disp;
static int g_w, g_h, g_gh, g_barh, g_open, g_backend = AMIGAGFX_BACKEND_AGA, g_video_mode;
static unsigned char g_pal[256 * 3];
static int g_verbose, g_ptr_x, g_ptr_y;
static unsigned long g_blits, g_blit_px, g_shots;
static char g_title[128];

typedef struct HostPtr { struct HostPtr *next; int w, h, hx, hy; unsigned char *pix; } HostPtr;
static HostPtr *g_ptrs;

/* ---- env helpers -------------------------------------------------------------------------- */
static long env_long(const char *name, long def)
{
	const char *s = getenv(name);
	return (s && *s) ? strtol(s, NULL, 0) : def;
}

void host_logf(const char *fmt, ...)
{
	va_list ap;
	fprintf(stderr, "[host %7lu] ", host_clock_us() / 1000);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fputc('\n', stderr);
	fflush(stderr);
}

/* ---- clock -------------------------------------------------------------------------------- */
static int g_clk_init, g_realtime;
static unsigned long long g_vclock_us;           /* virtual time */
static struct timespec g_t0;
static long g_poll_ms = 1, g_millis_us = 20;

static void clk_init(void)
{
	if (g_clk_init) return;
	g_clk_init = 1;
	g_realtime = (int)env_long("W2_REALTIME", 0);
	g_poll_ms = env_long("W2_POLL_MS", 1);
	g_millis_us = env_long("W2_MILLIS_US", 20);
	clock_gettime(CLOCK_MONOTONIC, &g_t0);
}

int host_realtime(void) { clk_init(); return g_realtime; }

unsigned long host_clock_us(void)
{
	clk_init();
	if (g_realtime) {
		struct timespec t;
		clock_gettime(CLOCK_MONOTONIC, &t);
		return (unsigned long)((t.tv_sec - g_t0.tv_sec) * 1000000LL + (t.tv_nsec - g_t0.tv_nsec) / 1000);
	}
	return (unsigned long)g_vclock_us;
}

void host_clock_advance_ms(unsigned long ms)
{
	clk_init();
	if (g_realtime) {
		struct timespec t;
		t.tv_sec = (time_t)(ms / 1000);
		t.tv_nsec = (long)(ms % 1000) * 1000000L;
		nanosleep(&t, NULL);
	} else {
		g_vclock_us += (unsigned long long)ms * 1000ULL;
	}
}

unsigned long amigagfx_millis(void)
{
	clk_init();
	if (!g_realtime) g_vclock_us += (unsigned long long)g_millis_us;
	return host_clock_us() / 1000;
}

/* ---- event queue -------------------------------------------------------------------------- */
#define EVQ 512
#define EV_INTERNAL_SHOT 100                     /* queue-only: take a shot when popped */
typedef struct { AmigaGfxEvent ev; char name[64]; } QEv;
static QEv g_q[EVQ];
static int g_qhead, g_qtail;

static void q_push(int type, int x, int y, int code, const char *name)
{
	int n = (g_qtail + 1) % EVQ;
	if (n == g_qhead) { host_logf("event queue full, dropped type %d", type); return; }
	g_q[g_qtail].ev.type = type; g_q[g_qtail].ev.x = x; g_q[g_qtail].ev.y = y; g_q[g_qtail].ev.code = code;
	snprintf(g_q[g_qtail].name, sizeof g_q[g_qtail].name, "%s", name ? name : "");
	g_qtail = n;
}

/* ---- raw key names (Amiga rawkey codes) --------------------------------------------------- */
typedef struct { const char *name; int code; } KeyName;
static const KeyName g_keys[] = {
	{"esc", 0x45}, {"escape", 0x45}, {"return", 0x44}, {"enter", 0x44}, {"kpenter", 0x43},
	{"space", 0x40}, {"backspace", 0x41}, {"tab", 0x42}, {"del", 0x46}, {"delete", 0x46},
	{"help", 0x5F}, {"up", 0x4C}, {"down", 0x4D}, {"right", 0x4E}, {"left", 0x4F},
	{"f1", 0x50}, {"f2", 0x51}, {"f3", 0x52}, {"f4", 0x53}, {"f5", 0x54},
	{"f6", 0x55}, {"f7", 0x56}, {"f8", 0x57}, {"f9", 0x58}, {"f10", 0x59},
	{"lshift", 0x60}, {"shift", 0x60}, {"rshift", 0x61}, {"caps", 0x62}, {"ctrl", 0x63},
	{"lalt", 0x64}, {"alt", 0x64}, {"ralt", 0x65}, {"lamiga", 0x66}, {"ramiga", 0x67},
	{"`", 0x00}, {"-", 0x0B}, {"=", 0x0C}, {"\\", 0x0D}, {"[", 0x1A}, {"]", 0x1B},
	{";", 0x29}, {"'", 0x2A}, {",", 0x38}, {".", 0x39}, {"/", 0x3A},
	{NULL, 0}
};

/* ASCII letter/digit -> rawkey, -1 if none */
static int raw_of_char(int c)
{
	static const char row1[] = "qwertyuiop", row2[] = "asdfghjkl", row3[] = "zxcvbnm";
	const char *p;
	c = tolower(c);
	if (c >= '1' && c <= '9') return 0x01 + (c - '1');
	if (c == '0') return 0x0A;
	if (c && (p = strchr(row1, c)) != NULL) return 0x10 + (int)(p - row1);
	if (c && (p = strchr(row2, c)) != NULL) return 0x20 + (int)(p - row2);
	if (c && (p = strchr(row3, c)) != NULL) return 0x31 + (int)(p - row3);
	return -1;
}

static int parse_key(const char *s)
{
	int i;
	char *e;
	long v;
	for (i = 0; g_keys[i].name; i++) if (!strcasecmp(s, g_keys[i].name)) return g_keys[i].code;
	if (s[0] && !s[1] && raw_of_char((unsigned char)s[0]) >= 0) return raw_of_char((unsigned char)s[0]);
	v = strtol(s, &e, 0);
	if (e != s && *e == 0 && v >= 0 && v <= 0xFF) return (int)v;
	return -1;
}

/* ---- input script ------------------------------------------------------------------------- */
typedef struct { unsigned long ms; char cmd[16]; char a1[64]; int x, y, nxy; } ScriptLine;
static ScriptLine *g_scr;
static int g_nscr, g_scrpos, g_scr_loaded;

static void script_load(void)
{
	const char *path = getenv("W2_INPUT");
	FILE *f;
	char line[256];
	unsigned long last = 0;
	int lineno = 0, cap = 0;
	g_scr_loaded = 1;
	if (!path || !*path) return;
	f = fopen(path, "r");
	if (!f) { host_logf("W2_INPUT: cannot open %s", path); return; }
	while (fgets(line, sizeof line, f)) {
		char *p = line, *e, w1[16], w2[64], w3[16], w4[16];
		unsigned long ms;
		int rel = 0, n;
		ScriptLine *L;
		lineno++;
		while (isspace((unsigned char)*p)) p++;
		if (*p == '#' || *p == 0) continue;
		if (!strncasecmp(p, "at ", 3)) p += 3;
		else if (!strncasecmp(p, "after ", 6)) { p += 6; rel = 1; }
		while (isspace((unsigned char)*p)) p++;
		if (*p == '+') { rel = 1; p++; }
		ms = strtoul(p, &e, 10);
		if (e == p) { host_logf("W2_INPUT line %d: no time: %s", lineno, line); continue; }
		if (rel) ms += last;
		last = ms;
		w1[0] = w2[0] = w3[0] = w4[0] = 0;
		n = sscanf(e, "%15s %63s %15s %15s", w1, w2, w3, w4);
		if (n < 1) continue;
		if (g_nscr == cap) {
			cap = cap ? cap * 2 : 64;
			g_scr = (ScriptLine *)realloc(g_scr, (size_t)cap * sizeof *g_scr);
			if (!g_scr) { fclose(f); g_nscr = 0; return; }
		}
		L = &g_scr[g_nscr];
		memset(L, 0, sizeof *L);
		L->ms = ms;
		snprintf(L->cmd, sizeof L->cmd, "%s", w1);
		snprintf(L->a1, sizeof L->a1, "%s", w2);
		{   /* numbers: "move x y" has them in w2/w3, "down left x y" in w3/w4 */
			char *e1, *e2;
			const char *sx = w2, *sy = w3;
			if (!strcasecmp(w1, "down") || !strcasecmp(w1, "up") || !strcasecmp(w1, "click")) { sx = w3; sy = w4; }
			L->x = (int)strtol(sx, &e1, 0);
			L->y = (int)strtol(sy, &e2, 0);
			L->nxy = (*sx && e1 != sx && *sy && e2 != sy) ? 2 : 0;
		}
		g_nscr++;
	}
	fclose(f);
	host_logf("W2_INPUT: %d actions from %s", g_nscr, path);
}

static int button_of(const char *s)
{
	return (!strcasecmp(s, "right") || !strcasecmp(s, "r") || !strcmp(s, "1")) ? AMIGAGFX_BUTTON_RIGHT : AMIGAGFX_BUTTON_LEFT;
}

static void script_feed(void)
{
	unsigned long now;
	if (!g_scr_loaded) script_load();
	now = host_clock_us() / 1000;
	while (g_scrpos < g_nscr && g_scr[g_scrpos].ms <= now) {
		ScriptLine *L = &g_scr[g_scrpos++];
		const char *c = L->cmd;
		if (g_verbose) host_logf("script: %s %s", c, L->a1);
		if (L->nxy) { g_ptr_x = L->x; g_ptr_y = L->y; }
		if (!strcasecmp(c, "move")) {
			q_push(AMIGAGFX_EV_MOUSEMOVE, g_ptr_x, g_ptr_y, 0, NULL);
		} else if (!strcasecmp(c, "down") || !strcasecmp(c, "up") || !strcasecmp(c, "click")) {
			int b = button_of(L->a1);
			if (L->nxy) q_push(AMIGAGFX_EV_MOUSEMOVE, g_ptr_x, g_ptr_y, 0, NULL);
			if (strcasecmp(c, "up")) q_push(AMIGAGFX_EV_MOUSEDOWN, g_ptr_x, g_ptr_y, b, NULL);
			if (strcasecmp(c, "down")) q_push(AMIGAGFX_EV_MOUSEUP, g_ptr_x, g_ptr_y, b, NULL);
		} else if (!strcasecmp(c, "key") || !strcasecmp(c, "keydown") || !strcasecmp(c, "keyup")) {
			int k = parse_key(L->a1);
			if (k < 0) { host_logf("script: unknown key '%s'", L->a1); continue; }
			if (strcasecmp(c, "keyup")) q_push(AMIGAGFX_EV_KEY, g_ptr_x, g_ptr_y, k & 0x7F, NULL);
			if (strcasecmp(c, "keydown")) q_push(AMIGAGFX_EV_KEY, g_ptr_x, g_ptr_y, (k & 0x7F) | 0x80, NULL);
		} else if (!strcasecmp(c, "type")) {             /* type <word>: each char a key press+release */
			const char *p;
			for (p = L->a1; *p; p++) {
				int k = raw_of_char((unsigned char)*p);
				if (k < 0) continue;
				q_push(AMIGAGFX_EV_KEY, g_ptr_x, g_ptr_y, k, NULL);
				q_push(AMIGAGFX_EV_KEY, g_ptr_x, g_ptr_y, k | 0x80, NULL);
			}
		} else if (!strcasecmp(c, "quit")) {
			q_push(AMIGAGFX_EV_QUIT, g_ptr_x, g_ptr_y, 0, NULL);
		} else if (!strcasecmp(c, "shot")) {
			q_push(EV_INTERNAL_SHOT, 0, 0, 0, L->a1[0] ? L->a1 : "shot");
		} else if (!strcasecmp(c, "exit")) {             /* hard exit, for scripts that must end */
			host_logf("script: exit");
			exit((int)strtol(L->a1, NULL, 0));
		} else {
			host_logf("script: unknown command '%s'", c);
		}
	}
}

/* ---- PNG writer (stored deflate, no zlib) ------------------------------------------------- */
static unsigned long g_crctab[256];

static unsigned long crc_update(unsigned long c, const unsigned char *b, size_t n)
{
	size_t i;
	if (!g_crctab[1]) {
		unsigned long k, v;
		int j;
		for (k = 0; k < 256; k++) {
			for (v = k, j = 0; j < 8; j++) v = (v & 1) ? 0xEDB88320UL ^ (v >> 1) : v >> 1;
			g_crctab[k] = v;
		}
	}
	for (i = 0; i < n; i++) c = g_crctab[(c ^ b[i]) & 0xFF] ^ (c >> 8);
	return c;
}

static void put32(unsigned char *p, unsigned long v)
{
	p[0] = (unsigned char)(v >> 24); p[1] = (unsigned char)(v >> 16); p[2] = (unsigned char)(v >> 8); p[3] = (unsigned char)v;
}

static void png_chunk(FILE *f, const char *type, const unsigned char *data, size_t n)
{
	unsigned char hdr[8], tail[4];
	unsigned long c;
	put32(hdr, (unsigned long)n);
	memcpy(hdr + 4, type, 4);
	fwrite(hdr, 1, 8, f);
	if (n) fwrite(data, 1, n, f);
	c = crc_update(0xFFFFFFFFUL, hdr + 4, 4);
	c = crc_update(c, data, n) ^ 0xFFFFFFFFUL;
	put32(tail, c);
	fwrite(tail, 1, 4, f);
}

/* rgb: w*h*3 bytes. Returns 0 on success. */
static int png_write(const char *path, const unsigned char *rgb, int w, int h)
{
	static const unsigned char sig[8] = {137, 80, 78, 71, 13, 10, 26, 10};
	size_t raw_n = (size_t)h * (size_t)(w * 3 + 1), nblk = (raw_n + 65534) / 65535, z_n, i, pos;
	unsigned char ihdr[13], *raw, *z, *q;
	unsigned long a = 1, b = 0;
	int y;
	FILE *f;
	raw = (unsigned char *)malloc(raw_n);
	z_n = 2 + raw_n + nblk * 5 + 4;
	z = (unsigned char *)malloc(z_n);
	if (!raw || !z) { free(raw); free(z); return 1; }
	for (y = 0; y < h; y++) {
		raw[(size_t)y * (w * 3 + 1)] = 0;
		memcpy(raw + (size_t)y * (w * 3 + 1) + 1, rgb + (size_t)y * w * 3, (size_t)w * 3);
	}
	q = z;
	*q++ = 0x78; *q++ = 0x01;
	for (pos = 0; pos < raw_n; ) {
		size_t n = raw_n - pos > 65535 ? 65535 : raw_n - pos;
		*q++ = (unsigned char)(pos + n == raw_n);
		*q++ = (unsigned char)n; *q++ = (unsigned char)(n >> 8);
		*q++ = (unsigned char)~n; *q++ = (unsigned char)(~n >> 8);
		memcpy(q, raw + pos, n);
		q += n; pos += n;
	}
	for (i = 0; i < raw_n; i++) { a = (a + raw[i]) % 65521; b = (b + a) % 65521; }
	put32(q, (b << 16) | a); q += 4;
	put32(ihdr, (unsigned long)w); put32(ihdr + 4, (unsigned long)h);
	ihdr[8] = 8; ihdr[9] = 2; ihdr[10] = 0; ihdr[11] = 0; ihdr[12] = 0;
	f = fopen(path, "wb");
	if (!f) { free(raw); free(z); return 1; }
	fwrite(sig, 1, 8, f);
	png_chunk(f, "IHDR", ihdr, 13);
	png_chunk(f, "IDAT", z, (size_t)(q - z));
	png_chunk(f, "IEND", NULL, 0);
	free(raw); free(z);
	return fclose(f) != 0;
}

static unsigned char *disp_rgb(void)
{
	unsigned char *rgb = (unsigned char *)malloc((size_t)g_w * g_gh * 3 + 1);
	size_t i;
	if (!rgb) return NULL;
	for (i = 0; i < (size_t)g_w * g_gh; i++) memcpy(rgb + i * 3, g_pal + g_disp[i] * 3, 3);
	return rgb;
}

const char *host_shotdir(void)
{
	const char *d = getenv("W2_SHOTDIR");
	return (d && *d) ? d : ".";
}

int host_shot(const char *name)
{
	char path[1024];
	unsigned char *rgb;
	int r;
	size_t n = strlen(name);
	if (!g_open) { host_logf("shot %s: no screen open", name); return 1; }
	mkdir(host_shotdir(), 0777);
	snprintf(path, sizeof path, "%s/%s%s", host_shotdir(), name, (n > 4 && !strcasecmp(name + n - 4, ".png")) ? "" : ".png");
	rgb = disp_rgb();
	if (!rgb) return 1;
	r = png_write(path, rgb, g_w, g_gh);
	free(rgb);
	g_shots++;
	host_logf("shot %s %s", path, r ? "FAILED" : "written");
	return r;
}

/* ---- optional SDL2 window ----------------------------------------------------------------- */
#ifdef HOST_SDL
static SDL_Window *g_win;
static SDL_Renderer *g_ren;
static SDL_Texture *g_tex;
static uint32_t *g_argb;
static int g_scale = 1;
static unsigned long g_last_present;

static int sdl_raw(SDL_Scancode s)
{
	const char *n;
	switch (s) {
	case SDL_SCANCODE_ESCAPE: return 0x45; case SDL_SCANCODE_RETURN: return 0x44; case SDL_SCANCODE_KP_ENTER: return 0x43;
	case SDL_SCANCODE_SPACE: return 0x40; case SDL_SCANCODE_BACKSPACE: return 0x41; case SDL_SCANCODE_TAB: return 0x42;
	case SDL_SCANCODE_DELETE: return 0x46; case SDL_SCANCODE_UP: return 0x4C; case SDL_SCANCODE_DOWN: return 0x4D;
	case SDL_SCANCODE_RIGHT: return 0x4E; case SDL_SCANCODE_LEFT: return 0x4F;
	case SDL_SCANCODE_LSHIFT: return 0x60; case SDL_SCANCODE_RSHIFT: return 0x61; case SDL_SCANCODE_CAPSLOCK: return 0x62;
	case SDL_SCANCODE_LCTRL: case SDL_SCANCODE_RCTRL: return 0x63; case SDL_SCANCODE_LALT: return 0x64;
	case SDL_SCANCODE_RALT: return 0x65; case SDL_SCANCODE_LGUI: return 0x66; case SDL_SCANCODE_RGUI: return 0x67;
	case SDL_SCANCODE_F11: return 0x5F;
	default: break;
	}
	if (s >= SDL_SCANCODE_F1 && s <= SDL_SCANCODE_F10) return 0x50 + (s - SDL_SCANCODE_F1);
	n = SDL_GetScancodeName(s);
	if (n && n[0] && !n[1]) { int k = parse_key((char[]){(char)tolower((unsigned char)n[0]), 0}); return k; }
	return -1;
}

static void sdl_open(void)
{
	const char *disp = getenv("DISPLAY"), *wl = getenv("WAYLAND_DISPLAY");
	if (env_long("W2_HEADLESS", 0) || ((!disp || !*disp) && (!wl || !*wl))) return;
	if (SDL_Init(SDL_INIT_VIDEO) != 0) { host_logf("SDL_Init: %s - headless", SDL_GetError()); return; }
	g_scale = (int)env_long("W2_SCALE", 1);
	if (g_scale < 1) g_scale = 1;
	g_win = SDL_CreateWindow(g_title[0] ? g_title : "w2 host", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
	                         g_w * g_scale, g_gh * g_scale, 0);
	if (g_win) g_ren = SDL_CreateRenderer(g_win, -1, 0);
	if (g_ren) g_tex = SDL_CreateTexture(g_ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, g_w, g_gh);
	g_argb = (uint32_t *)malloc((size_t)g_w * g_gh * 4);
	if (!g_tex || !g_argb) {
		host_logf("SDL window failed: %s - headless", SDL_GetError());
		if (g_ren) SDL_DestroyRenderer(g_ren);
		if (g_win) SDL_DestroyWindow(g_win);
		free(g_argb); g_argb = NULL; g_win = NULL; g_ren = NULL; g_tex = NULL;
		SDL_Quit();
		return;
	}
	SDL_ShowCursor(SDL_DISABLE);
	if (!getenv("W2_REALTIME")) g_realtime = 1;        /* a window is for playing: real time */
	host_logf("SDL window %dx%d scale %d", g_w, g_gh, g_scale);
}

static void sdl_present(int force)
{
	size_t i;
	unsigned long now = host_clock_us() / 1000;
	if (!g_tex) return;
	if (!force && now - g_last_present < 10) return;
	g_last_present = now;
	for (i = 0; i < (size_t)g_w * g_gh; i++) {
		const unsigned char *c = g_pal + g_disp[i] * 3;
		g_argb[i] = 0xFF000000u | ((uint32_t)c[0] << 16) | ((uint32_t)c[1] << 8) | c[2];
	}
	SDL_UpdateTexture(g_tex, NULL, g_argb, g_w * 4);
	SDL_RenderClear(g_ren);
	SDL_RenderCopy(g_ren, g_tex, NULL, NULL);
	SDL_RenderPresent(g_ren);
}

static void sdl_pump(void)
{
	SDL_Event e;
	if (!g_win) return;
	while (SDL_PollEvent(&e)) {
		switch (e.type) {
		case SDL_QUIT: q_push(AMIGAGFX_EV_QUIT, g_ptr_x, g_ptr_y, 0, NULL); break;
		case SDL_MOUSEMOTION:
			g_ptr_x = e.motion.x / g_scale; g_ptr_y = e.motion.y / g_scale;
			q_push(AMIGAGFX_EV_MOUSEMOVE, g_ptr_x, g_ptr_y, 0, NULL);
			break;
		case SDL_MOUSEBUTTONDOWN: case SDL_MOUSEBUTTONUP:
			if (e.button.button != SDL_BUTTON_LEFT && e.button.button != SDL_BUTTON_RIGHT) break;
			g_ptr_x = e.button.x / g_scale; g_ptr_y = e.button.y / g_scale;
			q_push(e.type == SDL_MOUSEBUTTONDOWN ? AMIGAGFX_EV_MOUSEDOWN : AMIGAGFX_EV_MOUSEUP, g_ptr_x, g_ptr_y,
			       e.button.button == SDL_BUTTON_RIGHT ? AMIGAGFX_BUTTON_RIGHT : AMIGAGFX_BUTTON_LEFT, NULL);
			break;
		case SDL_KEYDOWN: case SDL_KEYUP: {
			int k = sdl_raw(e.key.keysym.scancode);
			if (e.type == SDL_KEYDOWN && e.key.keysym.scancode == SDL_SCANCODE_F12) { host_shot("f12"); break; }
			if (k < 0 || (e.type == SDL_KEYDOWN && e.key.repeat)) break;
			q_push(AMIGAGFX_EV_KEY, g_ptr_x, g_ptr_y, e.type == SDL_KEYUP ? (k | 0x80) : k, NULL);
			break;
		}
		default: break;
		}
	}
}

static void sdl_close(void)
{
	if (!g_win) return;
	SDL_DestroyTexture(g_tex); SDL_DestroyRenderer(g_ren); SDL_DestroyWindow(g_win);
	free(g_argb);
	g_win = NULL; g_ren = NULL; g_tex = NULL; g_argb = NULL;
	SDL_Quit();
}
#endif

/* ---- timeout ------------------------------------------------------------------------------ */
static int g_quit_sent;
static void check_timeout(void)
{
	static long limit = -1;
	unsigned long now = host_clock_us() / 1000;
	if (limit < 0) limit = env_long("W2_TIMEOUT", 600000);
	if (limit == 0) return;
#ifdef HOST_SDL
	if (g_win && !getenv("W2_TIMEOUT")) return;
#endif
	if (!g_quit_sent && now >= (unsigned long)limit) {
		host_logf("W2_TIMEOUT %ld ms reached - sending QUIT", limit);
		q_push(AMIGAGFX_EV_QUIT, g_ptr_x, g_ptr_y, 0, NULL);
		g_quit_sent = 1;
	} else if (g_quit_sent && now >= (unsigned long)limit + 30000) {
		host_logf("W2_TIMEOUT: game ignored QUIT for 30 s - exit(3)");
		exit(3);
	}
}

/* ---- the API ------------------------------------------------------------------------------ */
void amigagfx_set_video_mode(int mode) { g_video_mode = mode; }
int  amigagfx_video_mode(void) { return g_video_mode; }
int  amigagfx_is_ntsc(void) { return g_video_mode == 2 || (g_video_mode == 0 && env_long("W2_NTSC", 0)); }
unsigned long amigagfx_paula_clock(void) { return amigagfx_is_ntsc() ? 3579545UL : 3546895UL; }
int  amigagfx_rtg_has_mode(int w, int h) { (void)w; (void)h; return (int)env_long("W2_RTG", 1); }
void amigagfx_set_rtg_depth(int bits) { (void)bits; }
void amigagfx_message(const char *text) { host_logf("MESSAGE: %s", text ? text : "(null)"); }
int  amigagfx_backend(void) { return g_backend; }
int  amigagfx_is_morphos(void) { return 0; }
int  amigagfx_palette_needs_redraw(void) { return 0; }
int  amigagfx_wb_available(void) { return 1; }
int  amigagfx_wb_colours(int *granted) { if (granted) *granted = 256; return AMIGAGFX_WBCOL_OWN; }
int  amigagfx_game_width(void) { return g_w; }
int  amigagfx_game_height(void) { return g_gh; }
void amigagfx_pointer_suspend(int on) { (void)on; }
int  amigagfx_screen_in_front(void) { return g_open ? 1 : -1; }
void amigagfx_screen_to_front(void) { }
void amigagfx_set_hide_system_pointer(int on) { (void)on; }
void amigagfx_pointer_colours(const unsigned char *rgb9) { (void)rgb9; }
unsigned char *amigagfx_chunky(void) { return g_chunky; }
int  amigagfx_pitch(void) { return g_w; }
void amigagfx_set_verbose(int verbose) { g_verbose = verbose; }
void amigagfx_log(const char *msg) { host_logf("%s", msg ? msg : "(null)"); }
void AmigaMemProbe(const char *label) { if (g_verbose) host_logf("mem probe %s", label); }
unsigned long AmigaFreeFastMem(void) { return 64UL << 20; }
unsigned long AmigaLargestFastMem(void) { return 32UL << 20; }

void amigagfx_set_screen_title(const char *title)
{
	snprintf(g_title, sizeof g_title, "%s", title ? title : "");
#ifdef HOST_SDL
	if (g_win) SDL_SetWindowTitle(g_win, g_title);
#endif
}

/* EHB checks (errors are counted, the first few logged, the total is in the exit line) */
static unsigned long g_ehb_bad_px, g_ehb_bad_reg;

static void atexit_stats(void)
{
	if (g_blits || g_shots)
		host_logf("exit: %lu blits, %lu px pushed, %lu shots, virtual clock %lu ms", g_blits, g_blit_px, g_shots,
		          host_clock_us() / 1000);
	if (g_backend == AMIGAGFX_BACKEND_EHB)
		host_logf("EHB check: %lu pixels >= 64 blitted, %lu palette calls beyond register 31%s", g_ehb_bad_px, g_ehb_bad_reg,
		          (g_ehb_bad_px || g_ehb_bad_reg) ? "  <-- ERRORS" : " (clean)");
}

int amigagfx_open(int w, int h, int show_bar, int backend)
{
	static int atexit_done;
	if (g_open) amigagfx_close();
	if (w <= 0 || h <= 0 || w > 4096 || h > 4096) return 1;
	if ((backend == AMIGAGFX_BACKEND_AGA || backend == AMIGAGFX_BACKEND_EHB) && (w & 31)) {
		host_logf("amigagfx_open: width %d not a multiple of 32 on AGA/EHB", w);
		return 1;
	}
	g_barh = (show_bar && backend != AMIGAGFX_BACKEND_WB) ? 11 : 0;
	g_w = w; g_h = h; g_gh = h - g_barh;
	g_chunky = (unsigned char *)calloc((size_t)w * g_gh, 1);   /* exact size: ASan sees overruns */
	g_disp = (unsigned char *)calloc((size_t)w * g_gh, 1);
	if (!g_chunky || !g_disp) { free(g_chunky); free(g_disp); g_chunky = g_disp = NULL; return 1; }
	g_backend = backend;
	g_open = 1;
	g_ptr_x = w / 2; g_ptr_y = g_gh / 2;
	if (!atexit_done) { atexit(atexit_stats); atexit_done = 1; }
	host_logf("amigagfx_open %dx%d bar=%d backend=%d -> game area %dx%d", w, h, show_bar, backend, g_w, g_gh);
#ifdef HOST_SDL
	sdl_open();
#endif
	return 0;
}

void amigagfx_close(void)
{
	HostPtr *p, *n;
	if (!g_open) return;
	if (getenv("W2_SHOTDIR") && env_long("W2_FINALSHOT", 1)) host_shot("final");
#ifdef HOST_SDL
	sdl_close();
#endif
	for (p = g_ptrs; p; p = n) { n = p->next; free(p->pix); free(p); }
	g_ptrs = NULL;
	free(g_chunky); free(g_disp);
	g_chunky = g_disp = NULL;
	g_open = 0;
	host_logf("amigagfx_close");
}

int amigagfx_dump_screen(const char *path)
{
	FILE *f;
	unsigned char *rgb;
	if (!g_open || !path) return 1;
	f = fopen(path, "wb");                       /* PROGDIR: mapped by host_sys.c's fopen wrapper */
	if (!f) return 1;
	rgb = disp_rgb();
	if (!rgb) { fclose(f); return 1; }
	fprintf(f, "P6\n%d %d\n255\n", g_w, g_gh);
	fwrite(rgb, 3, (size_t)g_w * g_gh, f);
	free(rgb);
	host_logf("dump_screen %s", path);
	return fclose(f) != 0;
}

void *amigagfx_pointer_new(const unsigned char *pix, int w, int h, int hotx, int hoty)
{
	HostPtr *p;
	if (!pix || w <= 0 || h <= 0 || w > 64) return NULL;
	p = (HostPtr *)calloc(1, sizeof *p);
	if (!p) return NULL;
	p->pix = (unsigned char *)malloc((size_t)w * h);
	if (!p->pix) { free(p); return NULL; }
	memcpy(p->pix, pix, (size_t)w * h);
	p->w = w; p->h = h; p->hx = hotx; p->hy = hoty;
	p->next = g_ptrs; g_ptrs = p;
	return p;
}

void amigagfx_pointer_use(void *p) { (void)p; }

void amigagfx_set_palette(const unsigned char *rgb, int first, int count)
{
	if (!rgb || first < 0 || count <= 0) return;
	if (first + count > 256) count = 256 - first;
	if (g_open && g_backend == AMIGAGFX_BACKEND_EHB) {
		/* Like the chipset: 32 registers of 4 bits per gun; index i+32 shows register i with every gun
		 * shifted right once. Setting a register >= 32 is a bug of the caller. */
		int i, k;
		if (first + count > 32) {
			if (g_ehb_bad_reg++ < 5) host_logf("EHB ERROR: set_palette(first %d, count %d) touches registers >= 32", first, count);
			count = 32 - first;
		}
		for (i = 0; i < count; i++)
			for (k = 0; k < 3; k++) {
				int n = rgb[i * 3 + k] >> 4;
				g_pal[(first + i) * 3 + k] = (unsigned char)(n * 17);
				g_pal[(first + i + 32) * 3 + k] = (unsigned char)((n >> 1) * 17);
			}
		for (i = 64; i < 256; i++) memcpy(g_pal + i * 3, g_pal + (i & 63) * 3, 3);   /* 6 bitplanes: only the low 6 bits exist */
#ifdef HOST_SDL
		sdl_present(1);
#endif
		return;
	}
	memcpy(g_pal + first * 3, rgb, (size_t)count * 3);
#ifdef HOST_SDL
	sdl_present(1);
#endif
}

void amigagfx_set_ehb_palette(const unsigned char *rgb64) { (void)rgb64; }

void amigagfx_blit(int x, int y, int w, int h)
{
	int r;
	if (!g_open) return;
	if (g_backend == AMIGAGFX_BACKEND_AGA || g_backend == AMIGAGFX_BACKEND_EHB) {   /* c2p 32-px columns */
		int x1 = (x + w + 31) & ~31;
		x &= ~31;
		w = x1 - x;
	}
	if (x < 0) { w += x; x = 0; }
	if (y < 0) { h += y; y = 0; }
	if (x + w > g_w) w = g_w - x;
	if (y + h > g_gh) h = g_gh - y;
	if (w <= 0 || h <= 0) return;
	for (r = 0; r < h; r++) memcpy(g_disp + (size_t)(y + r) * g_w + x, g_chunky + (size_t)(y + r) * g_w + x, (size_t)w);
	if (g_backend == AMIGAGFX_BACKEND_EHB) {      /* 6 bitplanes: a pixel >= 64 is a bug of the game */
		int c;
		for (r = 0; r < h; r++) {
			const unsigned char *p = g_chunky + (size_t)(y + r) * g_w + x;
			for (c = 0; c < w; c++) if (p[c] >= 64) {
				if (g_ehb_bad_px++ < 10) host_logf("EHB ERROR: pixel %d at %d,%d (blit %lu)", p[c], x + c, y + r, g_blits);
			}
		}
	}
	g_blits++;
	g_blit_px += (unsigned long)w * h;
	if (g_verbose && (g_blits % 500) == 0) host_logf("blit #%lu (%lu px total)", g_blits, g_blit_px);
#ifdef HOST_SDL
	sdl_present(0);
#endif
}

int amigagfx_poll(AmigaGfxEvent *ev)
{
	if (!ev) return 0;
#ifdef HOST_SDL
	sdl_pump();
#endif
	script_feed();
	check_timeout();
	while (g_qhead != g_qtail) {
		QEv *q = &g_q[g_qhead];
		g_qhead = (g_qhead + 1) % EVQ;
		if (q->ev.type == EV_INTERNAL_SHOT) { host_shot(q->name); continue; }
		*ev = q->ev;
		if (g_verbose) host_logf("event type %d x %d y %d code 0x%x", ev->type, ev->x, ev->y, ev->code);
		return 1;
	}
	ev->type = AMIGAGFX_EV_NONE;
	if (g_realtime) {
#ifdef HOST_SDL
		if (g_win) sdl_present(0);
#endif
	} else {
		g_vclock_us += (unsigned long long)g_poll_ms * 1000ULL;
	}
	return 0;
}

int amigagfx_warp_pointer(int x, int y)
{
	g_ptr_x = x; g_ptr_y = y;
	q_push(AMIGAGFX_EV_MOUSEMOVE, x, y, 0, NULL);   /* Intuition delivers one for the warp too */
	return 1;
}

void amigagfx_splash(const char *path)
{
	host_logf("splash %s (not shown on host)", path ? path : "(null)");
	if (g_chunky) memset(g_chunky, 0, (size_t)g_w * g_gh);
	memset(g_pal, 0, sizeof g_pal);
}
