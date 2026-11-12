/* snd.h - sound module extras beyond the [snd.c] block of w2.h (owner: snd.c).
 *
 * Sound effects: War Sounds entries (RIFF WAV, 8-bit unsigned 11025 Hz) loaded on demand into a small
 * LRU cache in Chip RAM (budget SND_CHIP_BUDGET), one effect per Paula channel; channels 0+1 while
 * music plays, 0..3 otherwise. World sounds are heard only inside / near the viewport.
 *
 * Music: the CD audio tracks of the player's CD, converted by tools/w2conv into
 * <data_dir>/music/trackNN.adp (IMA ADPCM mono) and streamed by the platform music streamer on
 * channels 2+3. Track use follows the original Mac game's music table:
 *   game     human 2..6 (tracks shorter than 15 s skipped - the Mac CD's track 2), orc 10..14, looping
 *   briefing human 7, orc 15 (looping)        victory human 8, orc 16        defeat human 9, orc 17
 *   menu     title.adp (the Mac "Title Song"), looping
 * Missing files = silence, never an error.
 */
#ifndef W2_SND_H
#define W2_SND_H

#include "w2.h"

#define SND_CHIP_BUDGET (160UL * 1024UL)   /* Chip RAM for cached effects */
#define SND_CACHE_SLOTS 48

enum { SND_MUS_GAME, SND_MUS_BRIEFING, SND_MUS_VICTORY, SND_MUS_DEFEAT, SND_MUS_MENU, SND_MUS_COUNT };

/* Start music of a kind for a race (0 human, 1 orc). snd_music(1) == snd_music_play(SND_MUS_GAME,
 * local player's race); snd_music(0) stops. Returns 1 if something is playing. */
int  snd_music_play(int kind, int race);
/* CD track number now streaming (0 = title song / nothing: see snd_music_active). */
int  snd_music_track(void);
int  snd_music_active(void);

/* counters for logs and tests */
typedef struct SndStats {
	unsigned long played, dropped_busy, dropped_far, dropped_missing, loads, evictions;
	unsigned long chip_used;
	unsigned last_key;          /* War Sounds entry (or 0x8000|War Data entry) of the last effect started */
	int cached;
} SndStats;
void snd_stats(SndStats *out);

#endif
