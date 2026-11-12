/* cdimage.h - CD image access for w2conv: CUE/BIN (raw 2352 or cooked 2048 sectors), plain ISO,
 * data-track byte reads (2048-byte user data) and CD-DA audio track reads.
 * PC-side tool code (Linux / Windows mingw); plain C99. */
#ifndef W2CONV_CDIMAGE_H
#define W2CONV_CDIMAGE_H

#include <stdio.h>
#include <stdint.h>

#define CD_MAX_TRACKS 99
#define CD_MAX_FILES  99

typedef struct CdTrack {
	int      number;      /* 1..99 */
	int      audio;       /* 1 = CD-DA */
	int      file;        /* index into CdImage.files */
	int      sector_size; /* 2352 or 2048 (bytes per sector in the file) */
	int      data_off;    /* user data offset inside a sector (16 MODE1/2352, 24 MODE2/2352, 0 cooked) */
	int      big_endian;  /* audio samples stored big-endian (FILE ... MOTOROLA) */
	int64_t  start;       /* first sector (INDEX 01) within the file */
	int64_t  index0;      /* INDEX 00 within the file, -1 if none */
	int64_t  length;      /* sectors */
} CdTrack;

typedef struct CdImage {
	int      nfiles;
	FILE    *f[CD_MAX_FILES];
	int64_t  fsize[CD_MAX_FILES];
	int      ntracks;
	CdTrack  t[CD_MAX_TRACKS];
	/* sector cache for data reads */
	int      cache_track;
	int64_t  cache_sector;
	unsigned char cache[2352];
} CdImage;

/* Open a .cue (with its BIN files), a raw .bin without cue (single data track guessed from the sync
 * pattern) or a cooked .iso. Returns 0 on success, prints the reason and returns -1 otherwise. */
int  cd_open(CdImage *cd, const char *path);
void cd_close(CdImage *cd);

/* Read len bytes of USER DATA of data track ti (index into cd->t) starting at byte offset off
 * (offset in 2048-byte logical sectors). 0 = ok. */
int  cd_read(CdImage *cd, int ti, int64_t off, void *dst, size_t len);
/* Byte size of the data track's user data. */
int64_t cd_track_bytes(const CdImage *cd, int ti);

/* Read 'frames' CD-DA frames (588 stereo samples each) of audio track ti starting at frame 'first'
 * into dst as interleaved signed 16-bit L,R host-order samples. Returns frames read. */
int  cd_read_audio(CdImage *cd, int ti, int64_t first, int frames, int16_t *dst);

/* Index of the first data track, -1 if none. */
int  cd_first_data_track(const CdImage *cd);

/* Portable 64-bit seek. */
int  w2c_seek(FILE *f, int64_t off);
int64_t w2c_fsize(FILE *f);

#endif
