/* assets.h - the game's data: archives, palettes, terrain, sprites, player colours, strings, fonts.
 *
 * Every module gets original data ONLY through here, so that lores (halved art) and caching live in
 * one place. Hires coordinates everywhere; in lores (assets_lores() != 0) the drawing helpers take
 * HIRES positions and halve them themselves; assets_megatile() returns 16x16 tiles and the sprite /
 * image data is halved once (filtered, lores.c) and drawn 1:1 - no per-frame scaling.
 */
#ifndef W2_ASSETS_H
#define W2_ASSETS_H

#include "war.h"
#include "w2gfx_decode.h"

enum { ASSET_DATA, ASSET_RESOURCES, ASSET_STRINGS, ASSET_SOUNDS, ASSET_SPEECH, ASSET_MOVIES, ASSET_COUNT };

int  assets_init(void);                 /* opens the archives from w2_cfg.data_dir; 0 = ok */
void assets_shutdown(void);
int  assets_lores(void);                /* 1 when running 320x240 */
/* EHB (ECS Extra-Half-Brite, 64 colours, always lores; ehb.h): set by the platform shell from the
 * backend that really opened, BEFORE game_init / assets_set_era / lores_prepare. In EHB every pixel
 * the game draws is 0..63, assets_palette() is the "virtual palette" (entry i = EHB colour i & 63),
 * the art is converted (ehb-<era>.pack) and the remap tables work on EHB indices. */
void assets_set_ehb(int on);
int  assets_ehb(void);
WarArchive *assets_war(int which);      /* may be NULL (e.g. no movies) */
unsigned char *assets_load(int which, int entry, unsigned long *size);   /* malloc'd, war_free() */

/* ---- era: palette + terrain ---- */
int  assets_set_era(int era);           /* 0 summer, 1 winter, 2 wasteland (3 swamp -> wasteland); always (re)loads + applies the era palette */
int  assets_era(void);
int  assets_era_entry(int era, int what);         /* War Data entry: what 0 palette 1 megatiles 2 minitiles 3 tilemap */
const unsigned char *assets_palette(void);         /* current 256x RGB, 8-bit per gun */
void assets_apply_palette(void);                   /* upload to the screen */
void assets_load_palette(int which, int entry);    /* a picture palette (menus) - becomes current */
/* The same, naming the art that will be shown under it: art = pairs (archive, entry) ended by -1; the
 * first is the picture, the others (panel image, raw widget sheet) weigh a quarter of it each. Only
 * EHB uses the list: it weighs the palette's 64 colours by those pixels. */
void assets_load_palette_for(int which, int entry, const short *art);
void assets_restore_era_palette(void);             /* the current era's palette again (after menu pictures) + apply */
int  assets_tile_size(void);                       /* 32 hires, 16 lores */
const unsigned char *assets_megatile(int mega);    /* tile_size x tile_size chunky block, NULL if bad */
int  assets_megatile_count(void);
int  assets_tile_to_mega(unsigned tile);           /* PUD MTXM value -> megatile, -1 if bad */
unsigned assets_tile_group_flags(unsigned tile);   /* tile map group flags (0x40 water, 0x01 land...) */

/* ---- sprites ---- */
typedef struct W2Spr {
	unsigned char *data; unsigned long size;
	int type;                 /* W2_SPR_RLE / W2_SPR_RAW */
	int count, box_w, box_h;  /* box_w/box_h are ALWAYS the hires box (lores data carries its own halved box) */
} W2Spr;
const W2Spr *assets_sprite(int which, int entry);  /* cached; NULL if missing; entry 0 = none */
const W2Spr *assets_sprite_any(int which, int entry); /* same, entry 0 allowed (War Resources 0/1 gfu) */
void assets_flush_sprites(void);                   /* drop the cache (era change) */

/* Player colour remap (256 bytes): 208..211 -> the player's colour; also keeps pointer pens 17..19
 * out of the picture. player 0..7; anything else = neutral (identity + pointer fix). */
const unsigned char *assets_remap(int player);
const unsigned char *assets_base_remap(void);
/* RAW source art (bytes of an archive entry under the current palette) -> screen index. Same as the
 * base remap except in EHB, where it is source index -> nearest EHB colour (the base remap there works
 * on already converted art). For modules that load and draw raw pictures themselves (ui.c, menu.c). */
const unsigned char *assets_src_map(void);

/* Draw a sprite frame centred like the original: (px,py) = HIRES map/screen position of the TOP-LEFT
 * of the frame box. flip = mirror. remap may be NULL (base remap is used). Handles lores halving. */
void assets_draw_sprite(const W2Spr *s, int frame, int flip, unsigned char *dst, int pitch,
                        int px, int py, const W2Clip *clip, const unsigned char *remap);
/* Opaque image entry (u16 w,h + pixels) at HIRES (px,py). Cached (lores: halved under the current palette). */
void assets_draw_image(int which, int entry, unsigned char *dst, int pitch, int px, int py, const W2Clip *clip);

/* ---- strings & fonts ---- */
const char *assets_string(int entry, int idx);     /* War Strings table entry, CP437, "" if missing */
const char *assets_unit_name(int type);
enum { FONT_LARGE_TITLE, FONT_SMALL_TITLE, FONT_LARGE, FONT_GAME, FONT_SMALL, FONT_COUNT };
enum { FC_YELLOW, FC_WHITE, FC_GREY, FC_RED, FC_GREEN, FC_BLUE, FC_BLACK, FC_COUNT };
/* Text at HIRES (px,py) top-left. Lores draws the 3x5 font instead. Returns advance (hires px). */
int  assets_text(int font, int colour, unsigned char *dst, int pitch, const W2Clip *clip, int px, int py, const char *s);
int  assets_text_width(int font, const char *s);
int  assets_font_height(int font);

#endif
