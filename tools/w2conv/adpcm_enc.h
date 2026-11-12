/* adpcm_enc.h - resampler + IMA ADPCM WAV writer for w2conv.
 * Output format = what native/amiga_adpcm.c streams: RIFF WAVE, wFormatTag 0x11, mono, 4-bit,
 * blockAlign 1024, 2041 samples per block (header sample + 2040 nibbles, low nibble first). */
#ifndef W2CONV_ADPCM_ENC_H
#define W2CONV_ADPCM_ENC_H

#include <stdint.h>

#define ADPE_BLOCK_ALIGN 1024
#define ADPE_SPB         2041

/* Resample mono 16-bit 'in' (n samples at in_rate) to out_rate with a windowed-sinc low-pass.
 * Returns a malloc'd buffer, *out_n = its sample count. */
int16_t *adpe_resample(const int16_t *in, long n, long in_rate, long out_rate, long *out_n);

/* Write samples as an IMA ADPCM WAV file. Returns the file size or -1. */
long adpe_write_wav(const char *path, const int16_t *s, long n, long rate);

/* Statistics of a buffer (for the log): peak |x| and rms. */
void adpe_stats(const int16_t *s, long n, int *peak, int *rms);

#endif
