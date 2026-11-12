/* w2cfg.c - PROGDIR:w2.cfg reader / writer, shared by w2 and w2setup. See w2cfg.h for the keys.
 * Plain C, stdio only (no Amiga headers, no floats, no sprintf). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "w2cfg.h"

W2Cfg w2_cfg;

/* unknown lines, kept verbatim (comments too) so a rewrite never loses a teammate's key */
#define XMAX 64
#define XLEN 160
static char g_extra[XMAX][XLEN];
static int g_nextra;

static const char *const k_backend[W2_BACKEND_COUNT] = { "auto", "aga", "rtg", "ehb", "wb" };
static const char *const k_audio[W2_AUDIO_COUNT] = { "paula", "ahi", "off" };

const char *w2cfg_backend_name(int b) { return b >= 0 && b < W2_BACKEND_COUNT ? k_backend[b] : "?"; }
const char *w2cfg_audio_name(int a) { return a >= 0 && a < W2_AUDIO_COUNT ? k_audio[a] : "?"; }

int w2cfg_screen_w(const W2Cfg *c) { return c->resolution == W2_RES_LORES ? 320 : 640; }
int w2cfg_screen_h(const W2Cfg *c) { return c->resolution == W2_RES_LORES ? 240 : 480; }

void w2cfg_defaults(W2Cfg *c)
{
	memset(c, 0, sizeof *c);
	c->backend = W2_BACKEND_AUTO;
	c->resolution = W2_RES_HIRES;
	c->rtg_depth = 0;
	c->titlebar = 0;
	c->audio = W2_AUDIO_PAULA;
	c->music = 1;
	c->sfx_volume = 80;
	c->music_volume = 70;
	c->game_speed = 3;
	c->logic_skip = 0;
	c->scroll_speed = 3;
	c->show_fps = 0;
	strcpy(c->data_dir, "PROGDIR:data");
}

static int clampi(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }

static int lookup(const char *v, const char *const *tab, int n, int def)
{
	int i;
	for (i = 0; i < n; i++) if (!strcmp(v, tab[i])) return i;
	return def;
}

static int onoff(const char *v, int def)
{
	if (!strcmp(v, "on") || !strcmp(v, "1") || !strcmp(v, "yes")) return 1;
	if (!strcmp(v, "off") || !strcmp(v, "0") || !strcmp(v, "no")) return 0;
	return def;
}

static void trim(char *s)
{
	char *e = s + strlen(s);
	while (e > s && (e[-1] == '\n' || e[-1] == '\r' || e[-1] == ' ' || e[-1] == '\t')) *--e = 0;
}

/* 1 = known key (applied), 0 = unknown */
static int apply(W2Cfg *c, const char *k, const char *v)
{
	if (!strcmp(k, "backend")) c->backend = lookup(v, k_backend, W2_BACKEND_COUNT, W2_BACKEND_AUTO);
	else if (!strcmp(k, "resolution")) c->resolution = !strcmp(v, "lores") || !strcmp(v, "320x240") ? W2_RES_LORES : W2_RES_HIRES;
	else if (!strcmp(k, "rtg_depth")) {
		int d = atoi(v);
		c->rtg_depth = (d == 8 || d == 15 || d == 16 || d == 24 || d == 32) ? d : 0;
	}
	else if (!strcmp(k, "titlebar")) c->titlebar = onoff(v, 0);
	else if (!strcmp(k, "audio")) c->audio = lookup(v, k_audio, W2_AUDIO_COUNT, W2_AUDIO_PAULA);
	else if (!strcmp(k, "music")) c->music = onoff(v, 1);
	else if (!strcmp(k, "sfx_volume")) c->sfx_volume = clampi(atoi(v), 0, 100);
	else if (!strcmp(k, "music_volume")) c->music_volume = clampi(atoi(v), 0, 100);
	else if (!strcmp(k, "game_speed")) c->game_speed = clampi(atoi(v), 1, 5);
	else if (!strcmp(k, "logic_skip")) c->logic_skip = clampi(atoi(v), 0, 8);
	else if (!strcmp(k, "scroll_speed")) c->scroll_speed = clampi(atoi(v), 1, 5);
	else if (!strcmp(k, "show_fps")) c->show_fps = onoff(v, 0);
	else if (!strcmp(k, "data_dir")) { if (*v) snprintf(c->data_dir, sizeof c->data_dir, "%s", v); }
	else return 0;
	return 1;
}

int w2cfg_load(W2Cfg *c, const char *path)
{
	FILE *f;
	char line[XLEN];
	w2cfg_defaults(c);
	g_nextra = 0;
	if ((f = fopen(path, "r")) == NULL) return -1;
	while (fgets(line, sizeof line, f)) {
		char *v, *k = line;
		trim(line);
		while (*k == ' ' || *k == '\t') k++;
		v = strchr(k, '=');
		if (*k && *k != '#' && v) {
			char key[32];
			size_t kl = (size_t)(v - k);
			while (kl > 0 && (k[kl - 1] == ' ' || k[kl - 1] == '\t')) kl--;
			v++;
			while (*v == ' ' || *v == '\t') v++;
			if (kl < sizeof key) {
				memcpy(key, k, kl); key[kl] = 0;
				if (apply(c, key, v)) continue;
			}
		}
		if (*k && g_nextra < XMAX) snprintf(g_extra[g_nextra++], XLEN, "%s", k);
	}
	fclose(f);
	return 0;
}

const char *w2cfg_load_default(W2Cfg *c)
{
	if (w2cfg_load(c, W2CFG_ENV_FILE) == 0) return W2CFG_ENV_FILE;
	if (w2cfg_load(c, W2CFG_FILE) == 0) return W2CFG_FILE;
	return NULL;
}

int w2cfg_save(const W2Cfg *c, const char *path)
{
	FILE *f = fopen(path, "w");
	int i;
	if (!f) return -1;
	fprintf(f, "backend=%s\n", w2cfg_backend_name(c->backend));
	fprintf(f, "resolution=%s\n", c->resolution == W2_RES_LORES ? "lores" : "hires");
	fprintf(f, "rtg_depth=%d\n", c->rtg_depth);
	fprintf(f, "titlebar=%d\n", c->titlebar ? 1 : 0);
	fprintf(f, "audio=%s\n", w2cfg_audio_name(c->audio));
	fprintf(f, "music=%s\n", c->music ? "on" : "off");
	fprintf(f, "sfx_volume=%d\n", c->sfx_volume);
	fprintf(f, "music_volume=%d\n", c->music_volume);
	fprintf(f, "game_speed=%d\n", c->game_speed);
	fprintf(f, "logic_skip=%d\n", c->logic_skip);
	fprintf(f, "scroll_speed=%d\n", c->scroll_speed);
	fprintf(f, "show_fps=%d\n", c->show_fps ? 1 : 0);
	fprintf(f, "data_dir=%s\n", c->data_dir);
	for (i = 0; i < g_nextra; i++) fprintf(f, "%s\n", g_extra[i]);
	fclose(f);
	return 0;
}

const char *w2cfg_extra(const char *key)
{
	size_t n = strlen(key);
	int i;
	for (i = 0; i < g_nextra; i++)
		if (!strncmp(g_extra[i], key, n)) {
			const char *p = g_extra[i] + n;
			while (*p == ' ' || *p == '\t') p++;
			if (*p == '=') { p++; while (*p == ' ' || *p == '\t') p++; return p; }
		}
	return NULL;
}

int w2cfg_extra_int(const char *key, int def)
{
	const char *v = w2cfg_extra(key);
	return v && *v ? atoi(v) : def;
}
