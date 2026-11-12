/* smk.h - Smacker (SMK2) video + audio decoder, plain C (gnu99).
 *
 * Port of AmiSC's amiga/glue_smk.h (C++, the same author's code, written from
 * the public description of the format), extended with palette-exact output
 * and audio track extraction (Huffman-DPCM and raw PCM).
 *
 * No floats, no Amiga headers, endian-neutral (every multi-byte value is read
 * a byte at a time), so the same file builds on the host and on 68k.
 *
 * Usage:
 *     SmkVideo v;
 *     if (smk_open(&v, bytes, size) == 0) {         // bytes must outlive v
 *         while (smk_next_frame(&v, SMK_DECODE_ALL) == 0) {
 *             use v.pixels (v.w * v.h palette indices),
 *                 v.palette (768 bytes, 8-bit RGB) - changed when v.palette_changed,
 *                 smk_audio(&v, track, &bytes) - this frame's PCM for a track;
 *             if (v.cur == v.frames - 1) break;     // or loop
 *         }
 *         smk_close(&v);
 *     }
 *
 * Frames are deltas: decode them in order (0, 1, 2 ...). smk_next_frame()
 * wraps to frame 0 (or to 1 after a ring frame) and clears the picture.
 *
 * Audio output per frame: 8-bit tracks are UNSIGNED 8-bit samples (Smacker's
 * own representation; xor 0x80 for Paula), 16-bit tracks are signed 16-bit in
 * the HOST's byte order. Stereo is interleaved L, R.
 *
 * Lookup-table speed-up identical to AmiSC (SMK_TABLE_BITS first level,
 * SMK_SUB_BITS sub-tables, SMK_TABLE_BUDGET entries per tree).
 */
#ifndef SMK_H
#define SMK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SMK_MAX_TRACKS 7

/* smk_decode / smk_next_frame flags */
#define SMK_DECODE_VIDEO 1
#define SMK_DECODE_AUDIO 2
#define SMK_DECODE_ALL   3

/* header flags (SmkVideo.flags) */
#define SMK_FLAG_RING        1
#define SMK_FLAG_Y_INTERLACE 2   /* show every line twice, odd lines black */
#define SMK_FLAG_Y_DOUBLE    4   /* show every line twice */

/* growable arrays (internal) */
typedef struct SmkI32 { int32_t *p; uint32_t n, cap; } SmkI32;
typedef struct SmkU32 { uint32_t *p; uint32_t n, cap; } SmkU32;

typedef struct SmkTree {        /* internal: a flattened Huffman tree + lookup */
	SmkI32 t;                   /* node: 0x80000000|left size; leaf: value */
	SmkU32 tab;                 /* lookup tables */
	int tb;                     /* width of the first table */
	uint32_t mask;
	int present;                /* small trees: 0 = every code is 0, no bits */
	int last[3];                /* big trees: recent-value cache leaves */
	int esc[3];
} SmkTree;

typedef struct SmkTrack {
	int present;                /* header says this track carries data */
	int compressed;             /* Huffman DPCM (else raw PCM) */
	int channels;               /* 1 or 2 */
	int bits;                   /* 8 or 16 */
	uint32_t rate;              /* samples per second */
	uint8_t *buf;               /* this frame's samples (see header comment) */
	uint32_t len;               /* bytes in buf for the current frame */
	uint32_t cap;
	uint32_t total;             /* bytes delivered since smk_open */
} SmkTrack;

typedef struct SmkVideo {
	const uint8_t *data;
	uint32_t size;
	int w, h;                   /* pixels as stored (before Y doubling) */
	int frames;                 /* without the ring frame */
	int total;                  /* frames + ring frame */
	uint32_t frame_us;          /* how long one frame stays on screen */
	uint32_t flags;             /* SMK_FLAG_* */
	uint32_t *frame_size;       /* low 2 bits are flags (bit 0 keyframe) */
	uint8_t *frame_type;        /* bit 0 palette, bit 1+t audio track t */
	uint32_t *frame_at;
	SmkTree mmap, mclr, full, type;
	SmkTree atree[4];           /* audio byte trees, rebuilt per chunk */
	SmkTrack track[SMK_MAX_TRACKS];
	uint8_t *pixels;            /* w * h */
	uint8_t palette[768];
	int palette_changed;
	int cur;                    /* frame now in pixels, -1 before the first */
	int ok;
	unsigned long errors;       /* audio chunks that failed to decode */
} SmkVideo;

/* 0 on success, < 0 on a bad / unsupported file (state is then closed). */
int smk_open(SmkVideo *v, const uint8_t *data, uint32_t size);
void smk_close(SmkVideo *v);

/* Decodes frame f (palette, video and/or audio, per `what`). 0 on success. */
int smk_decode(SmkVideo *v, int f, int what);
/* The frame after the current one, looping the way the file asks. */
int smk_next_frame(SmkVideo *v, int what);

/* This frame's audio for track t (NULL and *bytes = 0 when none). */
const uint8_t *smk_audio(const SmkVideo *v, int t, uint32_t *bytes);

#ifdef __cplusplus
}
#endif

#endif /* SMK_H */
