/* rng.c - the original's game RNG (docs/RE_NOTES.md: Mac 1.23 at 0x188016):
 * seed = seed * 0x015A4E35 + 1; result = (seed >> 16) & 0x7FFF. Every game starts with seed 0.
 * Every random decision of the simulation must come from here. */
#include "w2.h"

uint32_t w2_rand(void)
{
	w2.rng = w2.rng * 0x015A4E35u + 1u;
	return (w2.rng >> 16) & 0x7FFF;
}
