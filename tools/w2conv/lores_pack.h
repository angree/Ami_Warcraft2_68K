/* lores_pack.h - w2conv: build <outdir>/data/lores-{summer,winter,wasteland}.pack (ehb 0) or
 * <outdir>/data/ehb-{summer,winter,wasteland}.pack (ehb 1, the ECS Extra-Half-Brite art) with the
 * game's own src/lores.c (see lores_pack.c). 0 = all three packs written or already current. */
#ifndef W2CONV_LORES_PACK_H
#define W2CONV_LORES_PACK_H
int lores_packs_build(const char *outdir, int ehb);
#endif
