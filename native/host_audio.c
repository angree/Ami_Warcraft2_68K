/* host_audio.c - the AmigaAudio_* API (amiga_audio.h) on a Linux/WSL host (-DW2_HOST only).
 *
 * No sound device: every call is logged to stderr (W2_AUDIOLOG=0 silences it), channel busy/idle
 * follows the host clock (a sample "plays" for bytes * period / paula_clock seconds), and music is
 * pulled through the game's refill callback at the rate it would really be consumed.
 * W2_WAV=1 additionally mixes everything into W2_SHOTDIR/audio.wav (22050 Hz, 16-bit mono) at
 * AmigaAudio_Close (or exit), so the sound timing can be checked by ear or in an editor.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include "amiga_audio.h"
#include "amiga_gfx.h"

unsigned long host_clock_us(void);
void host_logf(const char *fmt, ...);
const char *host_shotdir(void);

#define WAV_RATE 22050
#define WAV_MAX_SECONDS 1200

static int g_open, g_ahi_pref, g_log = -1, g_wav = -1, g_atexit;

typedef struct { unsigned long long end_us; } Chan;
static Chan g_ch[AMIGA_AUDIO_CHANNELS];

static int *g_mix;                         /* accumulated mix, WAV_RATE samples/s */
static size_t g_mix_len, g_mix_cap;

/* music */
static int g_mus_on, g_mus_ended, g_mus_period, g_mus_chunk, g_mus_vol = 64;
static int (*g_mus_refill)(void *, signed char *, int);
static void *g_mus_ud;
static unsigned long long g_mus_head_us;   /* stream time up to which samples were pulled */
static signed char *g_mus_buf;

static void alog(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
static void alog(const char *fmt, ...)
{
	char b[256];
	va_list ap;
	if (g_log < 0) { const char *s = getenv("W2_AUDIOLOG"); g_log = (s && *s == '0') ? 0 : 1; }
	if (!g_log) return;
	va_start(ap, fmt);
	vsnprintf(b, sizeof b, fmt, ap);
	va_end(ap);
	host_logf("audio: %s", b);
}

static unsigned long long now_us(void) { return host_clock_us(); }

static unsigned long long dur_us(unsigned long samples, int period)
{
	return (unsigned long long)samples * (unsigned long long)period * 1000000ULL / amigagfx_paula_clock();
}

/* ---- WAV mix ------------------------------------------------------------------------------ */
static int wav_on(void)
{
	if (g_wav < 0) { const char *s = getenv("W2_WAV"); g_wav = (s && *s && *s != '0') ? 1 : 0; }
	return g_wav;
}

static void mix_in(unsigned long long start_us, const signed char *s, unsigned long n, int period, int vol)
{
	size_t first, count, i, need;
	unsigned long src_rate;
	if (!wav_on() || !s || !n || period <= 0) return;
	src_rate = amigagfx_paula_clock() / (unsigned long)period;
	first = (size_t)(start_us * WAV_RATE / 1000000ULL);
	count = (size_t)((unsigned long long)n * WAV_RATE / src_rate);
	need = first + count;
	if (need > (size_t)WAV_RATE * WAV_MAX_SECONDS) return;
	if (need > g_mix_cap) {
		size_t nc = g_mix_cap ? g_mix_cap : WAV_RATE * 10;
		int *nm;
		while (nc < need) nc *= 2;
		nm = (int *)realloc(g_mix, nc * sizeof *nm);
		if (!nm) return;
		memset(nm + g_mix_cap, 0, (nc - g_mix_cap) * sizeof *nm);
		g_mix = nm; g_mix_cap = nc;
	}
	for (i = 0; i < count; i++) {
		unsigned long sp = (unsigned long)((unsigned long long)i * src_rate / WAV_RATE);
		if (sp >= n) break;
		g_mix[first + i] += s[sp] * vol * 4;     /* 8-bit * 0..64 -> ~16-bit */
	}
	if (need > g_mix_len) g_mix_len = need;
}

static void put16(unsigned char *p, unsigned v) { p[0] = (unsigned char)v; p[1] = (unsigned char)(v >> 8); }
static void put32(unsigned char *p, unsigned long v) { put16(p, (unsigned)(v & 0xFFFF)); put16(p + 2, (unsigned)(v >> 16)); }

static void wav_write(void)
{
	char path[1024];
	unsigned char h[44];
	FILE *f;
	size_t i;
	if (!wav_on() || !g_mix_len) return;
	mkdir(host_shotdir(), 0777);
	snprintf(path, sizeof path, "%s/audio.wav", host_shotdir());
	f = fopen(path, "wb");
	if (!f) { host_logf("audio: cannot write %s", path); return; }
	memcpy(h, "RIFF", 4); put32(h + 4, (unsigned long)(36 + g_mix_len * 2)); memcpy(h + 8, "WAVEfmt ", 8);
	put32(h + 16, 16); put16(h + 20, 1); put16(h + 22, 1); put32(h + 24, WAV_RATE); put32(h + 28, WAV_RATE * 2);
	put16(h + 32, 2); put16(h + 34, 16); memcpy(h + 36, "data", 4); put32(h + 40, (unsigned long)(g_mix_len * 2));
	fwrite(h, 1, 44, f);
	for (i = 0; i < g_mix_len; i++) {
		int v = g_mix[i];
		unsigned char b[2];
		if (v > 32767) v = 32767;
		if (v < -32768) v = -32768;
		put16(b, (unsigned)(v & 0xFFFF));
		fwrite(b, 1, 2, f);
	}
	fclose(f);
	host_logf("audio: wrote %s (%lu ms)", path, (unsigned long)(g_mix_len * 1000 / WAV_RATE));
	free(g_mix); g_mix = NULL; g_mix_len = g_mix_cap = 0;
}

static void at_exit(void) { wav_write(); free(g_mus_buf); g_mus_buf = NULL; }

/* ---- API ---------------------------------------------------------------------------------- */
int AmigaAudio_Open(void)
{
	if (getenv("W2_NOAUDIO")) { alog("Open -> 0 (W2_NOAUDIO)"); return 0; }
	g_open = 1;
	memset(g_ch, 0, sizeof g_ch);
	if (!g_atexit) { atexit(at_exit); g_atexit = 1; }
	alog("Open -> 1%s", wav_on() ? " (mixing to audio.wav)" : "");
	return 1;
}

int  AmigaAudio_IsAHI(void) { return g_ahi_pref; }
void AmigaAudio_PreferAHI(int on) { g_ahi_pref = on ? 1 : 0; alog("PreferAHI %d", on); }

void AmigaAudio_Close(void)
{
	AmigaAudio_MusicStop();
	if (g_open) alog("Close");
	g_open = 0;
	wav_write();
}

void *AmigaAudio_AllocSample(unsigned long bytes)
{
	void *p = calloc(1, bytes ? bytes : 1);
	alog("AllocSample %lu -> %p", bytes, p);
	return p;
}

void AmigaAudio_FreeSample(void *p) { free(p); }

int AmigaAudio_ChannelIdle(int ch)
{
	if (ch < 0 || ch >= AMIGA_AUDIO_CHANNELS) return 0;
	return now_us() >= g_ch[ch].end_us;
}

void AmigaAudio_StopChannel(int ch)
{
	if (ch < 0 || ch >= AMIGA_AUDIO_CHANNELS) return;
	g_ch[ch].end_us = now_us();
	alog("StopChannel ch=%d", ch);
}

int AmigaAudio_Play(int ch, void *chipdata, unsigned long bytes, int period, int volume)
{
	unsigned long long t = now_us();
	if (!g_open || ch < 0 || ch >= AMIGA_AUDIO_CHANNELS || !chipdata || !bytes || (bytes & 1) || bytes > 131070
	    || period < 124 || volume < 0 || volume > 64) {
		alog("Play ch=%d bytes=%lu period=%d vol=%d -> 0 (bad args / closed)", ch, bytes, period, volume);
		return 0;
	}
	if (t < g_ch[ch].end_us) { alog("Play ch=%d -> 0 (busy)", ch); return 0; }
	g_ch[ch].end_us = t + dur_us(bytes, period);
	mix_in(t, (const signed char *)chipdata, bytes, period, volume);
	alog("Play ch=%d bytes=%lu period=%d vol=%d (%lu ms)", ch, bytes, period, volume,
	     (unsigned long)(dur_us(bytes, period) / 1000));
	return 1;
}

int AmigaAudio_MusicStart(int period, int chunk_samples, int (*refill)(void *, signed char *, int), void *ud)
{
	if (!g_open || !refill || period < 124) { alog("MusicStart -> 0"); return 0; }
	AmigaAudio_MusicStop();
	if (chunk_samples < 256) chunk_samples = 256;
	if (chunk_samples > 65534) chunk_samples = 65534;
	chunk_samples &= ~1;
	g_mus_buf = (signed char *)malloc((size_t)chunk_samples);
	if (!g_mus_buf) return 0;
	g_mus_period = period; g_mus_chunk = chunk_samples; g_mus_refill = refill; g_mus_ud = ud;
	g_mus_on = 1; g_mus_ended = 0;
	g_mus_head_us = now_us();
	alog("MusicStart period=%d chunk=%d", period, chunk_samples);
	AmigaAudio_MusicService();
	return 1;
}

void AmigaAudio_MusicStop(void)
{
	if (!g_mus_on) return;
	g_mus_on = 0;
	free(g_mus_buf); g_mus_buf = NULL;
	alog("MusicStop");
}

void AmigaAudio_MusicService(void)
{
	unsigned long long ahead;
	if (!g_mus_on || g_mus_ended) return;
	ahead = 2 * dur_us((unsigned long)g_mus_chunk, g_mus_period);  /* two buffers queued, like Paula */
	while (!g_mus_ended && g_mus_head_us < now_us() + ahead) {
		int n = g_mus_refill(g_mus_ud, g_mus_buf, g_mus_chunk);
		if (n <= 0) { g_mus_ended = 1; alog("music: stream ended"); break; }
		if (n > g_mus_chunk) n = g_mus_chunk;
		if (g_mus_head_us < now_us()) g_mus_head_us = now_us();     /* underrun: restart from now */
		mix_in(g_mus_head_us, g_mus_buf, (unsigned long)n, g_mus_period, g_mus_vol);
		g_mus_head_us += dur_us((unsigned long)n, g_mus_period);
	}
}

void AmigaAudio_MusicSetVolume(int volume)
{
	g_mus_vol = volume < 0 ? 0 : volume > 64 ? 64 : volume;
	alog("MusicSetVolume %d", g_mus_vol);
}

int AmigaAudio_MusicActive(void) { return g_mus_on; }
int AmigaAudio_MusicFinished(void) { return g_mus_on && g_mus_ended && now_us() >= g_mus_head_us; }
int AmigaAudio_MusicThreadStart(int pri) { alog("MusicThreadStart %d -> 0 (host: MusicService per frame)", pri); return 0; }
void AmigaAudio_MusicThreadStop(void) { }
unsigned long AmigaAudio_MusicUnderruns(void) { return 0; }
int AmigaAudio_MusicThreadRunning(void) { return 0; }
