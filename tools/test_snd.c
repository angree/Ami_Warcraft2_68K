/* test_snd.c - host test of src/snd.c (sound effects, unit voices, CD-audio music).
 *
 * Runs a scripted ~20 s timeline on the host audio backend (native/host_audio.c, virtual clock),
 * which mixes every Play/music buffer into $W2_SHOTDIR/audio.wav (W2_WAV=1). Afterwards the WAV is
 * read back and analysed: duration, peak, rms per 500 ms window, clipped samples, and the music part
 * must be non-silent. Data: $W2_DATA (a w2conv output "data" folder with music/).
 * Build + run: tools/build_test_snd.sh. Exit code 0 = all checks passed.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "w2.h"
#include "w2cfg.h"
#include "assets.h"
#include "snd.h"
#include "amiga_audio.h"
#include "w2_sys.h"

static int g_fail;

void w2_log(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	printf("[w2_log] ");
	vprintf(fmt, ap);
	printf("\n");
	va_end(ap);
	fflush(stdout);
}

#define CHECK(c, ...) do { if (c) printf("PASS: " __VA_ARGS__); else { printf("FAIL: " __VA_ARGS__); g_fail++; } printf("\n"); } while (0)

static void run_ms(unsigned long ms)
{
	unsigned long t;
	for (t = 0; t < ms; t += 20) { w2_wait_tof(); snd_service(); AmigaAudio_MusicService(); }
}

static int make_unit(int slot, int type, int owner, int tx, int ty)
{
	W2Unit *u = &w2.unit[slot];
	memset(u, 0, sizeof *u);
	u->flags = W2_UF_ALIVE; u->type = (uint8_t)type; u->owner = (uint8_t)owner; u->tx = (uint8_t)tx; u->ty = (uint8_t)ty;
	if (slot + 1 > w2.unit_hi) w2.unit_hi = (uint16_t)(slot + 1);
	return slot;
}

static int analyse(const char *path, unsigned long music_from_ms, unsigned long music_to_ms)
{
	FILE *f = fopen(path, "rb");
	unsigned char h[44];
	long n, i, clipped = 0, peak = 0, rate;
	short *s;
	double e = 0;
	int silent_music_windows = 0, music_windows = 0;
	if (!f) { printf("FAIL: no %s\n", path); return -1; }
	if (fread(h, 1, 44, f) != 44 || memcmp(h, "RIFF", 4)) { fclose(f); printf("FAIL: bad wav\n"); return -1; }
	rate = h[24] | h[25] << 8 | (long)h[26] << 16;
	n = (h[40] | h[41] << 8 | (long)h[42] << 16 | (long)h[43] << 24) / 2;
	s = (short *)malloc(sizeof(short) * (size_t)(n ? n : 1));
	if (!s || fread(s, 2, (size_t)n, f) != (size_t)n) { fclose(f); free(s); printf("FAIL: short wav\n"); return -1; }
	fclose(f);
	for (i = 0; i < n; i++) {
		long v = s[i] < 0 ? -(long)s[i] : s[i];
		if (v > peak) peak = v;
		if (v >= 32767) clipped++;
		e += (double)s[i] * s[i];
	}
	printf("wav: %s  %ld Hz  %ld samples = %ld ms  peak %ld  rms %d  clipped %ld (%.3f%%)\n", path, rate, n, n * 1000 / rate,
	       peak, (int)__builtin_sqrt(e / (n ? n : 1)), clipped, n ? 100.0 * clipped / n : 0.0);
	printf("rms per 500 ms window:\n");
	for (i = 0; i + rate / 2 <= n; i += rate / 2) {
		long j, ms = i * 1000 / rate;
		double w = 0;
		for (j = i; j < i + rate / 2; j++) w += (double)s[j] * s[j];
		w = __builtin_sqrt(w / (rate / 2));
		printf("  %6ld ms %6d%s\n", ms, (int)w, ms >= (long)music_from_ms && ms + 500 <= (long)music_to_ms ? "  (music)" : "");
		if (ms >= (long)music_from_ms + 500 && ms + 500 <= (long)music_to_ms) { music_windows++; if (w < 200) silent_music_windows++; }
	}
	CHECK(n * 1000 / rate >= 18000 && n * 1000 / rate <= 21000, "wav duration %ld ms (expected ~19-20 s)", n * 1000 / rate);
	CHECK(peak > 3000, "wav not silent (peak %ld)", peak);
	CHECK(clipped * 1000 < n, "no clipping storm (%ld clipped samples)", clipped);
	CHECK(music_windows >= 18 && silent_music_windows <= 1, "music part non-silent: %d of %d windows quiet", silent_music_windows, music_windows);
	free(s);
	return 0;
}

int main(void)
{
	const char *data = getenv("W2_DATA");
	const char *shot = getenv("W2_SHOTDIR");
	char wav[1024];
	SndStats st;
	int foot, grunt, i, track;
	unsigned long t0, music_from, music_to;

	w2cfg_defaults(&w2_cfg);
	snprintf(w2_cfg.data_dir, sizeof w2_cfg.data_dir, "%s", data ? data : "/mnt/d/GITHUB/_AmiW2-poza-repo/gamedata/conv/data");
	w2_cfg.music = 1; w2_cfg.sfx_volume = 80; w2_cfg.music_volume = 70;
	if (!AmigaAudio_Open()) { printf("FAIL: audio open\n"); return 1; }   /* the shell does this */
	if (assets_init()) { printf("FAIL: assets_init (%s)\n", w2_cfg.data_dir); return 1; }
	snd_init();
	view.map_x = 0; view.map_y = 0; view.scr_w = 640; view.scr_h = 400;
	w2.local_player = 0;
	w2.player[0].race = 0; w2.player[1].race = 1;
	foot = make_unit(0, W2U_FOOTMAN, 0, 5, 5);
	grunt = make_unit(1, W2U_GRUNT, 1, 8, 6);
	t0 = amigagfx_millis();

	printf("--- voices + effects (no music: channels 0..3)\n");
	run_ms(100);
	snd_unit(foot, 0);                     /* selected */
	run_ms(1400);
	snd_unit(foot, 1);                     /* acknowledge */
	run_ms(1000);
	snd_play(22, 320, 200);                /* explosion in view */
	snd_play(135, 600, 100);               /* sword, right side, at the same time: second channel */
	run_ms(100);
	snd_stats(&st);
	{
		unsigned long far0 = st.dropped_far;
		snd_play(22, 5000, 5000);          /* far away: not heard */
		snd_stats(&st);
		CHECK(st.dropped_far == far0 + 1, "sound far outside the viewport is not played");
	}
	run_ms(400);
	{
		/* 12 quick clicks: after 6 the "annoyed" lines (War Sounds 17,19,..,29 for human soldiers) */
		int annoyed = 0, k;
		unsigned last = 0;
		printf("clicks:");
		for (i = 0; i < 12; i++) {
			snd_unit(foot, 0);
			snd_stats(&st);
			if (st.last_key != last) {
				last = st.last_key;
				printf(" %u", last);
				for (k = 0; k < 7; k++) if (last == w2_sound_entries[w2_sounds[1].first + k]) annoyed++;
			}
			run_ms(250);
		}
		printf("\n");
		CHECK(annoyed >= 1, "annoyed voice lines after many clicks (%d heard)", annoyed);
	}
	snd_play(20, -1, -1);                  /* UI click: War Data 432 */
	snd_stats(&st);
	CHECK(st.last_key == (0x8000u | 432u), "UI click plays War Data 432 (last key %#x)", st.last_key);
	snd_unit(grunt, 4);                    /* enemy dies in view: positional */
	run_ms(500);
	snd_stats(&st);
	printf("stats: played %lu dropped busy %lu far %lu missing %lu, loads %lu, cached %d, chip %lu bytes\n",
	       st.played, st.dropped_busy, st.dropped_far, st.dropped_missing, st.loads, st.cached, st.chip_used);
	CHECK(st.played >= 6, "effects/voices played (%lu)", st.played);
	CHECK(st.dropped_missing == 0, "no missing sound entries");
	CHECK(st.chip_used <= SND_CHIP_BUDGET, "chip budget respected (%lu)", st.chip_used);

	printf("--- music: human in-game playlist, 10 s, effects during music on channels 0+1\n");
	music_from = amigagfx_millis() - t0;
	snd_music(1);
	track = snd_music_track();
	CHECK(snd_music_active() && track == 3, "human game music starts at CD track 3 (got %d; the Mac CD track 2 is silent)", track);
	run_ms(1500);
	snd_play(135, 320, 200);
	snd_play(22, 100, 200);
	snd_play(140, 500, 200);               /* third at once: channels 0+1 only -> dropped */
	run_ms(8500);
	music_to = amigagfx_millis() - t0;
	snd_music(0);
	run_ms(300);
	snd_music_play(SND_MUS_VICTORY, 1);
	CHECK(snd_music_track() == 16, "orc victory = CD track 16 (got %d)", snd_music_track());
	run_ms(1500);
	snd_music(0);
	snd_stats(&st);
	printf("stats: played %lu dropped busy %lu far %lu missing %lu, loads %lu, cached %d, chip %lu bytes\n",
	       st.played, st.dropped_busy, st.dropped_far, st.dropped_missing, st.loads, st.cached, st.chip_used);
	snd_shutdown();
	assets_shutdown();
	AmigaAudio_Close();                    /* writes audio.wav */
	snprintf(wav, sizeof wav, "%s/audio.wav", shot ? shot : ".");
	analyse(wav, music_from, music_to);
	printf(g_fail ? "test_snd: %d FAILED\n" : "test_snd: all passed\n", g_fail);
	return g_fail ? 1 : 0;
}
