/* host_selftest.c - stand-in main for the host build (build/build-host.sh) while src/main_amiga.c
 * does not exist or cannot be built for the host. Exercises the platform API only:
 * opens 640x480, sets a palette, draws a gradient, follows the scripted mouse with a cross,
 * shifts the gradient on each click, plays a short beep, writes PROGDIR:selftest.txt, and quits on the
 * script's "quit" (or, with no W2_INPUT script, takes shot "selftest" and quits by itself). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "amiga_gfx.h"
#include "amiga_audio.h"

int host_shot(const char *name);

#define W 640
#define H 480

static void draw(unsigned char *d, int pitch, int t, int mx, int my)
{
	int x, y;
	for (y = 0; y < H; y++)
		for (x = 0; x < W; x++)
			d[y * pitch + x] = (unsigned char)(((x * 128) / W + (y * 128) / H + t) & 0xFF);
	for (x = mx - 8; x <= mx + 8; x++) if (x >= 0 && x < W && my >= 0 && my < H) d[my * pitch + x] = 255;
	for (y = my - 8; y <= my + 8; y++) if (y >= 0 && y < H && mx >= 0 && mx < W) d[y * pitch + mx] = 255;
}

int main(void)
{
	unsigned char pal[256 * 3];
	unsigned char *d;
	int i, pitch, quit = 0, mx = W / 2, my = H / 2, clicks = 0, keys = 0, scripted = getenv("W2_INPUT") != NULL;
	unsigned long t0, frames = 0;
	signed char *beep;
	FILE *f;

	if (amigagfx_open(W, H, 0, AMIGAGFX_BACKEND_RTG)) { fprintf(stderr, "selftest: open failed\n"); return 20; }
	for (i = 0; i < 256; i++) {                  /* blue -> red -> yellow ramp, 255 = white */
		pal[i * 3 + 0] = (unsigned char)(i < 128 ? i * 2 : 255);
		pal[i * 3 + 1] = (unsigned char)(i < 128 ? 0 : (i - 128) * 2);
		pal[i * 3 + 2] = (unsigned char)(i < 128 ? 255 - i * 2 : 0);
	}
	pal[255 * 3] = pal[255 * 3 + 1] = pal[255 * 3 + 2] = 255;
	amigagfx_set_palette(pal, 0, 256);
	d = amigagfx_chunky();
	pitch = amigagfx_pitch();

	if (AmigaAudio_Open()) {
		beep = (signed char *)AmigaAudio_AllocSample(2000);
		for (i = 0; i < 2000; i++) beep[i] = (signed char)((i & 16) ? 60 : -60);
		AmigaAudio_Play(0, beep, 2000, 254, 64);
	} else beep = NULL;

	f = fopen("PROGDIR:selftest.txt", "w");
	if (f) { fputs("host selftest was here\n", f); fclose(f); }
	else fprintf(stderr, "selftest: PROGDIR: mapping failed\n");

	t0 = amigagfx_millis();
	while (!quit) {
		AmigaGfxEvent ev;
		while (amigagfx_poll(&ev)) {
			if (ev.type == AMIGAGFX_EV_QUIT) quit = 1;
			else if (ev.type == AMIGAGFX_EV_MOUSEMOVE) { mx = ev.x; my = ev.y; }
			else if (ev.type == AMIGAGFX_EV_MOUSEDOWN) clicks++;
			else if (ev.type == AMIGAGFX_EV_KEY) { keys++; if (ev.code == 0x45) quit = 1; }
		}
		draw(d, pitch, clicks * 32, mx, my);
		amigagfx_blit(0, 0, W, H);
		frames++;
		if (!scripted && amigagfx_millis() - t0 > 1000) { host_shot("selftest"); quit = 1; }
	}
	{
		char b[128];
		snprintf(b, sizeof b, "selftest: %lu frames, %d clicks, %d key events, %lu ms", frames, clicks, keys,
		         amigagfx_millis() - t0);
		amigagfx_log(b);
	}
	if (beep) { while (!AmigaAudio_ChannelIdle(0)) { AmigaGfxEvent ev; amigagfx_poll(&ev); } AmigaAudio_FreeSample(beep); }
	AmigaAudio_Close();
	amigagfx_close();
	return 0;
}
