/* snd.c - sound effects, unit voices and CD-audio music (the [snd.c] block of w2.h + snd.h).
 *
 * Platform: native/amiga_audio.h only (Paula audio.device, or AHI on MorphOS - same API).
 *  - Effects: War Sounds entries (RIFF WAV mono 8-bit unsigned 11025 Hz; a few 16-bit) are parsed by
 *    their fmt/data chunks, converted to signed 8-bit and kept in an LRU cache in Chip RAM
 *    (SND_CHIP_BUDGET). One effect per channel, played by DMA; channels 0+1 while music streams,
 *    0..3 otherwise. Busy channels cannot be interrupted through the platform API, so when every
 *    allowed channel is busy the new effect is dropped (counted in SndStats).
 *  - World sounds are heard inside the viewport (full volume) or within SND_NEAR_PX of it (half),
 *    panned to the left/right Paula channels by their x position on screen.
 *  - Music: <data_dir>/music/trackNN.adp (IMA ADPCM, made by tools/w2conv from the CD audio) is
 *    decoded by native/amiga_adpcm.c inside the streamer's refill callback; the next track of the
 *    playlist is opened in the same callback, so tracks follow each other without a gap.
 * No floats, no simulation state: the variant counters use a private RNG, never w2_rand().
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "w2.h"
#include "w2cfg.h"
#include "assets.h"
#include "snd.h"
#include "amiga_audio.h"
#include "amiga_adpcm.h"
#include "w2_sys.h"

#ifdef W2_HOST
/* The host build may not link native/amiga_adpcm.c: then music is simply unavailable. */
#pragma weak Adpcm_Open
#pragma weak Adpcm_Close
#pragma weak Adpcm_Rate
#pragma weak Adpcm_Decode
#define ADPCM_LINKED() (Adpcm_Open != 0)
#else
#define ADPCM_LINKED() 1
#endif

#define SND_NEAR_PX      128            /* "near the viewport" margin, map pixels */
#define SND_SAME_MS      120            /* the same sound id is not restarted within this time */
#define SND_ANNOY_CLICKS 6              /* clicks on the same unit before the annoyed lines */
#define SND_CLICK_MS     1500           /* clicks closer than this count as "again" */
#define SND_HELP_MS      5000           /* "help" (under attack) at most this often */
#define MUS_CHUNK        2048           /* samples per music buffer (16 buffers in Chip RAM) */
#define MUS_MIN_SECONDS  15             /* shorter tracks are not part of the battle playlist */
#define WARDATA_KEY      0x8000u        /* cache key flag: War Data entry instead of War Sounds */

typedef struct Smp {
	signed char  *data;     /* Chip RAM, NULL = free slot */
	unsigned long len;      /* bytes, even */
	unsigned long used;     /* LRU stamp */
	uint16_t      key;      /* War Sounds entry, or WARDATA_KEY | War Data entry */
	uint16_t      period;
} Smp;

static int s_on;                        /* audio device usable */
static Smp s_cache[SND_CACHE_SLOTS];
static unsigned long s_chip, s_stamp;
static uint8_t s_bad[(512 + 7) / 8];    /* War Sounds entries that failed to load */
static int s_ch_smp[AMIGA_AUDIO_CHANNELS];
static int s_ch_id[AMIGA_AUDIO_CHANNELS];
static unsigned long s_ch_t[AMIGA_AUDIO_CHANNELS];
static uint8_t s_variant[256];
static uint32_t s_rng = 0x2F6E2B1u;
static SndStats s_st;

/* voices */
static int s_voice_ch = -1;
static int s_last_slot = -1, s_clicks;
static unsigned long s_last_click, s_last_help;
static int s_help_once;

/* music */
static AdpcmStream *s_mus, *s_mus_next;   /* current stream; next one when its rate differs */
static int s_mus_list[16], s_mus_n, s_mus_pos, s_mus_loop, s_mus_rate;
static int s_mus_playing, s_mus_track, s_mus_vol = -1;

static unsigned long now_ms(void) { return amigagfx_millis(); }
static unsigned rnd(void) { s_rng = s_rng * 1103515245u + 12345u; return (unsigned)(s_rng >> 16) & 0x7FFF; }

/* --------------------------------------------------------------------------------- samples */
static unsigned rd16(const unsigned char *p) { return (unsigned)p[0] | (unsigned)p[1] << 8; }
static unsigned long rd32(const unsigned char *p) { return (unsigned long)rd16(p) | (unsigned long)rd16(p + 2) << 16; }

static int ch_busy(int ch) { return s_ch_smp[ch] >= 0 && !AmigaAudio_ChannelIdle(ch); }

static int smp_in_use(int idx)
{
	int ch;
	for (ch = 0; ch < AMIGA_AUDIO_CHANNELS; ch++)
		if (s_ch_smp[ch] == idx) { if (ch_busy(ch)) return 1; s_ch_smp[ch] = -1; }
	return 0;
}

static void smp_free(int i)
{
	if (!s_cache[i].data) return;
	AmigaAudio_FreeSample(s_cache[i].data);
	s_chip -= s_cache[i].len;
	s_cache[i].data = NULL;
	s_st.evictions++;
}

/* drop the least recently used sample that is not playing; 0 if none could go */
static int evict_one(void)
{
	int i, best = -1;
	for (i = 0; i < SND_CACHE_SLOTS; i++)
		if (s_cache[i].data && !smp_in_use(i) && (best < 0 || s_cache[i].used < s_cache[best].used)) best = i;
	if (best < 0) return 0;
	smp_free(best);
	return 1;
}

/* Parse a RIFF WAV and convert its PCM to signed 8-bit mono into a new cache slot. */
static int smp_load(unsigned key)
{
	unsigned long size = 0, p, data_len = 0, frames, out_len, i, rate = 0;
	const unsigned char *data = NULL;
	unsigned char *raw;
	unsigned tag = 0, chans = 0, bits = 0, bpf;
	signed char *dst;
	int slot = -1, k;
	raw = (key & WARDATA_KEY) ? assets_load(ASSET_DATA, (int)(key & 0x7FFF), &size) : assets_load(ASSET_SOUNDS, (int)key, &size);
	if (!raw) return -1;
	if (size < 12 || memcmp(raw, "RIFF", 4) || memcmp(raw + 8, "WAVE", 4)) { war_free(raw); return -1; }
	for (p = 12; p + 8 <= size; ) {
		unsigned long cl = rd32(raw + p + 4);
		if (!memcmp(raw + p, "fmt ", 4) && cl >= 16 && p + 8 + 16 <= size) {
			tag = rd16(raw + p + 8); chans = rd16(raw + p + 10); rate = rd32(raw + p + 12); bits = rd16(raw + p + 22);
		} else if (!memcmp(raw + p, "data", 4)) {
			data = raw + p + 8;
			data_len = cl;
			if (data_len > size - (p + 8)) data_len = size - (p + 8);
			break;
		}
		p += 8 + cl + (cl & 1);
	}
	if (!data || tag != 1 || chans < 1 || chans > 2 || (bits != 8 && bits != 16) || rate < 2000) { war_free(raw); return -1; }
	bpf = chans * (bits / 8);
	frames = data_len / bpf;
	out_len = (frames + 1) & ~1UL;
	if (out_len > 131070UL) { out_len = 131070UL; frames = out_len; }
	if (out_len < 2) { war_free(raw); return -1; }
	/* make room: budget first, then a free slot */
	while (s_chip + out_len > SND_CHIP_BUDGET) if (!evict_one()) { war_free(raw); return -1; }
	for (k = 0; k < SND_CACHE_SLOTS; k++) if (!s_cache[k].data) { slot = k; break; }
	if (slot < 0) { if (!evict_one()) { war_free(raw); return -1; } for (k = 0; k < SND_CACHE_SLOTS; k++) if (!s_cache[k].data) { slot = k; break; } }
	dst = (signed char *)AmigaAudio_AllocSample(out_len);
	while (!dst && evict_one()) dst = (signed char *)AmigaAudio_AllocSample(out_len);
	if (!dst || slot < 0) { if (dst) AmigaAudio_FreeSample(dst); war_free(raw); return -1; }
	for (i = 0; i < frames; i++) {
		const unsigned char *s = data + i * bpf;
		int v;
		if (bits == 8) { v = (int)s[0] - 128; if (chans == 2) v = (v + (int)s[1] - 128) >> 1; }
		else { v = (int)(signed char)s[1]; if (chans == 2) v = (v + (int)(signed char)s[3]) >> 1; }
		dst[i] = (signed char)v;
	}
	for (; i < out_len; i++) dst[i] = 0;
	war_free(raw);
	s_cache[slot].data = dst;
	s_cache[slot].len = out_len;
	s_cache[slot].key = (uint16_t)key;
	{
		unsigned long per = (amigagfx_paula_clock() + rate / 2) / rate;
		if (per < 124) per = 124;
		if (per > 65535) per = 65535;
		s_cache[slot].period = (uint16_t)per;
	}
	s_chip += out_len;
	s_st.loads++;
	return slot;
}

static int smp_get(unsigned key)
{
	int i;
	for (i = 0; i < SND_CACHE_SLOTS; i++)
		if (s_cache[i].data && s_cache[i].key == key) { s_cache[i].used = ++s_stamp; return i; }
	if (!(key & WARDATA_KEY) && key < 512 && (s_bad[key >> 3] & (1u << (key & 7)))) return -1;
	i = smp_load(key);
	if (i < 0) {
		if (!(key & WARDATA_KEY) && key < 512) s_bad[key >> 3] |= (uint8_t)(1u << (key & 7));
		return -1;
	}
	s_cache[i].used = ++s_stamp;
	return i;
}

/* --------------------------------------------------------------------------------- playing */
/* pan: -1 left, 0 centre, 1 right. Channels 0/3 = left jack, 1/2 = right jack. */
static int pick_channel(int pan)
{
	static const int order[3][4] = { { 0, 3, 1, 2 }, { 0, 1, 3, 2 }, { 1, 2, 0, 3 } };
	int music = AmigaAudio_MusicActive(), i;
	for (i = 0; i < 4; i++) {
		int ch = order[pan + 1][i];
		if (music && ch >= 2) continue;
		if (s_ch_smp[ch] < 0 || AmigaAudio_ChannelIdle(ch)) return ch;
	}
	return -1;
}

static int play_key(unsigned key, int id, int vol64, int pan)
{
	int smp, ch, vol;
	if (vol64 <= 0) return -1;
	ch = pick_channel(pan);
	if (ch < 0) { s_st.dropped_busy++; return -1; }
	smp = smp_get(key);
	if (smp < 0) { s_st.dropped_missing++; return -1; }
	vol = w2_cfg.sfx_volume * vol64 / 100;
	if (vol > 64) vol = 64;
	if (vol <= 0) return -1;
	if (!AmigaAudio_Play(ch, s_cache[smp].data, s_cache[smp].len, s_cache[smp].period, vol)) { s_st.dropped_busy++; return -1; }
	s_ch_smp[ch] = smp;
	s_ch_id[ch] = id;
	s_ch_t[ch] = now_ms();
	s_st.last_key = key;
	s_st.played++;
	return ch;
}

/* gain 0..64 for a world position, -1 = not heard; *pan from the screen position */
static int world_gain(int px, int py, int *pan)
{
	int x0 = view.map_x, y0 = view.map_y, w = view.scr_w, h = view.scr_h;
	*pan = 0;
	if (px < 0) return 64;                      /* UI sound */
	if (w <= 0 || h <= 0) return 64;            /* no viewport yet */
	if (px < x0 + w / 3) *pan = -1;
	else if (px >= x0 + w - w / 3) *pan = 1;
	if (px >= x0 && px < x0 + w && py >= y0 && py < y0 + h) return 64;
	if (px >= x0 - SND_NEAR_PX && px < x0 + w + SND_NEAR_PX && py >= y0 - SND_NEAR_PX && py < y0 + h + SND_NEAR_PX) return 32;
	return -1;
}

/* War Sounds entry (or War Data key) of variant 'k' of sound id; 0 = none */
static unsigned sound_key(int id, int k)
{
	const w2_sound *g = &w2_sounds[id];
	unsigned e;
	if (!g->count) return 0;
	e = w2_sound_entries[g->first + (unsigned)k % g->count];
	if (e == 0xFFFF) return id == 20 ? (WARDATA_KEY | 432u) : 0;   /* "click" lives in War Data 432 */
	return e;
}

static int play_id(int id, int px, int py, int variant)
{
	int pan, g, ch;
	unsigned key;
	unsigned long t = now_ms();
	if (!s_on || id < 0 || id >= (int)w2_num_sounds || w2_cfg.sfx_volume <= 0) return -1;
	g = world_gain(px, py, &pan);
	if (g < 0) { s_st.dropped_far++; return -1; }
	for (ch = 0; ch < AMIGA_AUDIO_CHANNELS; ch++)   /* a burst of the same sound plays once */
		if (s_ch_id[ch] == id && t - s_ch_t[ch] < SND_SAME_MS && ch_busy(ch)) { s_st.dropped_busy++; return -1; }
	if (variant < 0) {
		unsigned c = w2_sounds[id].count;
		variant = c > 1 ? (int)(s_variant[id]++ % c) : 0;
	}
	key = sound_key(id, variant);
	if (!key) { s_st.dropped_missing++; return -1; }
	return play_key(key, id, g, pan);
}

/* --------------------------------------------------------------------------------- API (w2.h) */
int snd_init(void)
{
	int i;
	memset(s_cache, 0, sizeof s_cache);
	memset(&s_st, 0, sizeof s_st);
	memset(s_bad, 0, sizeof s_bad);
	for (i = 0; i < AMIGA_AUDIO_CHANNELS; i++) { s_ch_smp[i] = -1; s_ch_id[i] = -1; }
	s_chip = 0; s_stamp = 0;
	s_rng ^= (uint32_t)now_ms();
	for (i = 0; i < 256; i++) s_variant[i] = (uint8_t)rnd();
	s_on = 0;
	if (w2_cfg.audio == W2_AUDIO_OFF) { w2_log("snd: audio off"); return 0; }
	s_on = AmigaAudio_Open();      /* already opened by the shell: returns 1 at once */
	w2_log("snd: %s, effects cache %lu KB, music %s", s_on ? (AmigaAudio_IsAHI() ? "ahi.device" : "Paula") : "no audio device - silent",
	       SND_CHIP_BUDGET / 1024, !s_on ? "off" : !ADPCM_LINKED() ? "unavailable (no decoder)" : w2_cfg.music ? "on" : "off (settings)");
	return 0;
}

static void mus_close_all(void);

void snd_shutdown(void)
{
	int i, guard;
	if (s_mus_playing) AmigaAudio_MusicStop();
	s_mus_playing = 0;
	mus_close_all();
	if (s_on) {
		/* Paula keeps reading a sample until it ends: wait (max ~2 s) before freeing Chip RAM */
		for (guard = 0; guard < 100; guard++) {
			int busy = 0;
			for (i = 0; i < AMIGA_AUDIO_CHANNELS; i++) if (ch_busy(i)) busy = 1;
			if (!busy) break;
			w2_sleep_ms(20);
		}
	}
	for (i = 0; i < SND_CACHE_SLOTS; i++) smp_free(i);
	for (i = 0; i < AMIGA_AUDIO_CHANNELS; i++) s_ch_smp[i] = -1;
	w2_log("snd: shutdown - played %lu, dropped busy %lu far %lu missing %lu, loads %lu, music underruns %lu",
	       s_st.played, s_st.dropped_busy, s_st.dropped_far, s_st.dropped_missing, s_st.loads,
	       AmigaAudio_MusicUnderruns());
	s_on = 0;
}

void snd_play(int sound_id, int px, int py)
{
	play_id(sound_id, px, py, -1);
}

void snd_unit(int slot, int what)
{
	const W2Unit *u;
	const w2_unit_type *t;
	int id, px = -1, py = -1, variant = -1;
	unsigned long tm;
	if (!s_on || slot < 0 || slot >= W2_MAX_UNITS) return;
	u = &w2.unit[slot];
	t = w2_unit(u->type);
	if (!t) return;
	switch (what) {
	case 0: id = t->snd_selected; break;
	case 1: id = t->snd_ack; break;
	case 2: id = t->snd_ready; break;
	case 3: id = t->snd_help; break;
	case 4: id = t->snd_dead; break;
	default: return;
	}
	if (id == W2_NONE || id >= (int)w2_num_sounds) return;
	tm = now_ms();
	if (what == 4) {                                /* death: positional, anyone's unit */
		unit_center(slot, &px, &py);
		play_id(id, px, py, -1);
		return;
	}
	if (u->owner != w2.local_player) return;        /* voices only for our own units */
	if (what == 3) {
		if (s_help_once && tm - s_last_help < SND_HELP_MS) return;
		s_last_help = tm; s_help_once = 1;
	}
	if (what == 0) {                                /* selection: annoyed after many clicks */
		const w2_sound *g = &w2_sounds[id];
		if (slot == s_last_slot && tm - s_last_click < SND_CLICK_MS) s_clicks++;
		else s_clicks = 1;
		s_last_slot = slot; s_last_click = tm;
		if (g->annoyed != W2_NONE && s_clicks > SND_ANNOY_CLICKS) {
			int k = s_clicks - SND_ANNOY_CLICKS - 1;
			id = g->annoyed;
			if (k >= (int)w2_sounds[id].count) { s_clicks = 0; id = g - w2_sounds; }   /* all said: start over */
			else variant = k;                                                          /* in order */
		}
	}
	/* one voice at a time: never pile voices on top of each other */
	if (s_voice_ch >= 0 && ch_busy(s_voice_ch) && s_ch_id[s_voice_ch] >= 0) { s_st.dropped_busy++; return; }
	{
		int ch = play_id(id, -1, -1, variant);
		if (ch >= 0) s_voice_ch = ch;
	}
}

/* --------------------------------------------------------------------------------- music */
static void mus_path(char *buf, int cap, int track)
{
	const char *dir = w2_cfg.data_dir[0] ? w2_cfg.data_dir : "PROGDIR:data";
	size_t n = strlen(dir);
	const char *sep = (n && dir[n - 1] != ':' && dir[n - 1] != '/') ? "/" : "";
	if (track > 0) snprintf(buf, (size_t)cap, "%s%smusic/track%02d.adp", dir, sep, track);
	else snprintf(buf, (size_t)cap, "%s%smusic/title.adp", dir, sep);
}

/* seconds of audio in an .adp file from its header, -1 if missing / not ours */
static long adp_seconds(int track)
{
	char path[200];
	unsigned char h[64];
	FILE *f;
	long n, p = 12, rate = 0, data = -1;
	unsigned spb = 0, align = 0;
	mus_path(path, sizeof path, track);
	f = fopen(path, "rb");
	if (!f) return -1;
	n = (long)fread(h, 1, sizeof h, f);
	fclose(f);
	if (n < 44 || memcmp(h, "RIFF", 4) || memcmp(h + 8, "WAVE", 4)) return -1;
	while (p + 8 <= n) {
		unsigned long cl = rd32(h + p + 4);
		if (!memcmp(h + p, "fmt ", 4) && p + 28 <= n) {
			rate = (long)rd32(h + p + 12); align = rd16(h + p + 20);
			spb = (cl >= 20 && p + 28 <= n) ? rd16(h + p + 26) : 0;
		} else if (!memcmp(h + p, "data", 4)) { data = (long)cl; break; }
		p += 8 + (long)cl + (long)(cl & 1);
	}
	if (rate <= 0 || data < 0 || !align || !spb) return -1;
	return (long)((unsigned long)data / align * spb / (unsigned long)rate);
}

static void mus_close_all(void)
{
	if (s_mus && ADPCM_LINKED()) Adpcm_Close(s_mus);
	if (s_mus_next && ADPCM_LINKED()) Adpcm_Close(s_mus_next);
	s_mus = s_mus_next = NULL;
	s_mus_track = 0;
}

static AdpcmStream *mus_open(int track)
{
	char path[200];
	mus_path(path, sizeof path, track);
	return ADPCM_LINKED() ? Adpcm_Open(path) : NULL;
}

/* advance the playlist; opens the next stream. 0 = end of playlist */
static AdpcmStream *mus_advance(int *track)
{
	int tries;
	for (tries = 0; tries < s_mus_n; tries++) {
		AdpcmStream *s;
		s_mus_pos++;
		if (s_mus_pos >= s_mus_n) { if (!s_mus_loop) return NULL; s_mus_pos = 0; }
		s = mus_open(s_mus_list[s_mus_pos]);
		if (s) { *track = s_mus_list[s_mus_pos]; return s; }
	}
	return NULL;
}

static int mus_refill(void *ud, signed char *dst, int max)
{
	int n, guard;
	(void)ud;
	if (!s_mus || s_mus_next) return 0;
	for (guard = 0; guard < 2; guard++) {
		int track = 0;
		AdpcmStream *nx;
		n = Adpcm_Decode(s_mus, dst, max);
		if (n > 0) return n;
		nx = mus_advance(&track);
		if (!nx) return 0;                         /* playlist done */
		if (Adpcm_Rate(nx) != s_mus_rate) {          /* needs a new period: snd_service restarts */
			s_mus_next = nx; s_mus_track = track;
			return 0;
		}
		Adpcm_Close(s_mus);
		s_mus = nx; s_mus_track = track;
	}
	return 0;
}

static int mus_start_stream(void)
{
	unsigned long per;
	if (!s_mus) return 0;
	s_mus_rate = Adpcm_Rate(s_mus);
	if (s_mus_rate <= 0) return 0;
	per = (amigagfx_paula_clock() + (unsigned long)s_mus_rate / 2) / (unsigned long)s_mus_rate;
	if (per < 124) per = 124;
	s_mus_vol = w2_cfg.music_volume * 64 / 100;
	AmigaAudio_MusicSetVolume(s_mus_vol);
	if (!AmigaAudio_MusicStart((int)per, MUS_CHUNK, mus_refill, NULL)) return 0;
	s_mus_playing = 1;
	return 1;
}

int snd_music_play(int kind, int race)
{
	int i, n = 0, first, last;
	if (s_mus_playing) AmigaAudio_MusicStop();
	s_mus_playing = 0;
	mus_close_all();
	if (!s_on || !w2_cfg.music || w2_cfg.music_volume <= 0 || !ADPCM_LINKED() || kind < 0 || kind >= SND_MUS_COUNT) return 0;
	race = race ? 1 : 0;
	s_mus_loop = 1;
	switch (kind) {
	case SND_MUS_GAME:     first = race ? 10 : 2; last = race ? 14 : 6; break;
	case SND_MUS_BRIEFING: first = last = race ? 15 : 7; break;
	case SND_MUS_VICTORY:  first = last = race ? 16 : 8; s_mus_loop = 0; break;
	case SND_MUS_DEFEAT:   first = last = race ? 17 : 9; s_mus_loop = 0; break;
	default:               first = last = 0; break;          /* menu: title.adp */
	}
	for (i = first; i <= last && n < 16; i++) {
		long sec = adp_seconds(i);
		if (sec < 0) continue;
		if (kind == SND_MUS_GAME && sec < MUS_MIN_SECONDS) continue;   /* Mac CD track 2: 6 s of silence */
		s_mus_list[n++] = i;
	}
	s_mus_n = n;
	if (!n) { w2_log("snd: no music files for kind %d (run w2conv with the CD audio)", kind); return 0; }
	s_mus_pos = 0;
	s_mus = mus_open(s_mus_list[0]);
	if (!s_mus) return 0;
	s_mus_track = s_mus_list[0];
	if (!mus_start_stream()) { mus_close_all(); return 0; }
	w2_log("snd: music kind %d race %d, %d track(s), first track %d at %d Hz", kind, race, n, s_mus_track, s_mus_rate);
	return 1;
}

void snd_music(int on)
{
	if (!on) {
		if (s_mus_playing) AmigaAudio_MusicStop();
		s_mus_playing = 0;
		mus_close_all();
		return;
	}
	snd_music_play(SND_MUS_GAME, w2.player[w2.local_player & 15].race);
}

int snd_music_track(void) { return s_mus_playing ? s_mus_track : 0; }
int snd_music_active(void) { return s_mus_playing; }

void snd_service(void)
{
	if (!s_on) return;
	if (s_mus_playing) {
		int v = w2_cfg.music_volume * 64 / 100;
		if (v != s_mus_vol) { s_mus_vol = v; AmigaAudio_MusicSetVolume(v); }
		AmigaAudio_MusicService();
		if (AmigaAudio_MusicFinished()) {
			AmigaAudio_MusicStop();
			s_mus_playing = 0;
			if (s_mus_next) {                      /* next track has another rate: restart */
				Adpcm_Close(s_mus);
				s_mus = s_mus_next; s_mus_next = NULL;
				if (!mus_start_stream()) mus_close_all();
			} else mus_close_all();
		}
	}
}

void snd_stats(SndStats *out)
{
	int i;
	*out = s_st;
	out->chip_used = s_chip;
	out->cached = 0;
	for (i = 0; i < SND_CACHE_SLOTS; i++) if (s_cache[i].data) out->cached++;
}
