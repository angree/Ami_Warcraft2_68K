/* ehb.h - the 64-colour palette of the ECS Extra-Half-Brite mode (backend=ehb, always 320x240).
 *
 * An EHB screen has 32 settable colour registers (12-bit: 4 bits per gun on OCS/ECS) and 32 hardware
 * copies at half brightness: index i+32 shows register i with every 4-bit gun shifted right once.
 * This module turns a 256-colour source palette S into such a palette E (32 registers -> 64 colours)
 * with an integer weighted k-means in which every source colour may be served by a register or by its
 * half. Deterministic, integers only: the PC converter (tools/w2conv) and the game on the Amiga
 * compute identical bytes. An era's E (weighted by its megatiles and sprites) is built with the
 * era's pack and stored at its end; a picture's E is computed when its palette is loaded.
 *
 * Fixed registers:
 *   0          black
 *   17, 18, 19 the hardware pointer's colours on OCS/ECS (shared with the playfield): dark, mid, bright.
 *              Era palette: 17 = the BLACK player's colour, 18 = grey (152,152,152), 19 = the WHITE
 *              player's colour. Picture palette: (40,40,40), (152,152,152), (240,240,240).
 *   era palette only: one register per player (ehb_player_reg[]). The 4-shade player ramp becomes
 *              2 shades: the two brighter ramp positions = the register, the two darker = its half.
 * The other registers (22 in an era palette, 28 in a picture palette) are free for the k-means.
 *
 * Player colour inside sprites: ramp pixels are stored as the PLACEHOLDER pair EHB_PH / EHB_PH + 32
 * (one player's own register - the violet player's, the hue the art needs least) and every remap table
 * sends that pair to the drawing player's register. No other pixel of converted art (sprites, tiles,
 * images) may therefore use the placeholder pair; raw UI art drawn without a remap may.
 */
#ifndef W2_EHB_H
#define W2_EHB_H

#define EHB_VERSION 8          /* part of the pack stamp: bump when the palette algorithm changes */
#define EHB_PH_PLAYER 3        /* whose register is the placeholder */
extern const unsigned char ehb_player_reg[8];
#define EHB_PH (ehb_player_reg[EHB_PH_PLAYER])

/* source index of shade c (0..3) of player p's ramp in an era palette */
#define EHB_RAMP(p, c) ((p) == 7 ? 12 + (c) : 208 + (p) * 4 + (c))

/* 1 when ramp positions 0,1 are the brighter pair (they are in the original palettes) */
int ehb_bright_first(const unsigned char *src768);

/* Build E (64 x RGB, 8-bit per gun as DISPLAYED: registers are multiples of 17, halves the hardware's)
 * for the source palette src768. hist_a / hist_b: pixel counts per source index of two bodies of art
 * (an era: its megatiles and its sprites; a picture: the picture) - each is normalised to the same
 * total, so they weigh the same however large; NULL = not given, both NULL = every colour the same.
 * era_mode 1: player registers + placeholder rule; 0: picture. */
void ehb_build(const unsigned char *src768, const unsigned long *hist_a, const unsigned long *hist_b, int era_mode,
               unsigned char *e192);

/* nearest of the 64 colours to r,g,b (weights 3,4,2; first minimum); deny_ph: never the placeholder pair */
int ehb_nearest(const unsigned char *e192, int r, int g, int b, int deny_ph);

/* The "virtual palette": 256 entries, entry i shows E[i & 63], so the game's own nearest-colour
 * searches (which keep the FIRST minimum) answer 0..63. Entries >= 64 that would repeat 17..19 repeat
 * entry 1 instead, so a search that skips the pointer pens 17..19 cannot land on their copies. */
void ehb_virtual(const unsigned char *e192, unsigned char *pal768);

/* statistics of the last ehb_build: weighted mean squared distance (3,4,2 weights) of the source
 * colours to E after the initialisation and at the end */
extern unsigned long ehb_err_init, ehb_err_final;
extern unsigned long ehb_evals;          /* full assignment passes (256 colours x 64 entries) it took */

#endif
