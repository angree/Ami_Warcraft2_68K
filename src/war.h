/*
 * war.h - Warcraft II data archive reader (PC *.WAR / Mac "War Data" etc.)
 *
 * Our own implementation of the format described in docs/DATA_FORMAT.md.
 * Plain C (gnu99), stdio only (fopen/fread/fseek), no floats, endian-neutral:
 * works unchanged on big-endian 68k (libnix) and little-endian hosts.
 *
 * Archive layout (all little-endian):
 *   u32 magic = 0x19, u16 count, u16 archive_id, u32 offset[count]
 *   entry: u32 (size & 0xFFFFFF) | (flags << 24), then payload
 *          flags 0x00 = stored, 0x20 = LZSS compressed (see war.c)
 *   An entry whose offset is less than 4 bytes before the next entry (or EOF)
 *   is a placeholder: it has no data (war_load returns NULL, size 0).
 */
#ifndef W2_WAR_H
#define W2_WAR_H

#ifdef __cplusplus
extern "C" {
#endif

/* Known archive ids (u16 at offset 6 of the header). */
#define WAR_ID_DATA       1000  /* "War Data"      == MAINDAT.WAR  */
#define WAR_ID_SPEECH     2000  /* "War Music"     == SNDDAT.WAR (briefing speech!) */
#define WAR_ID_RESOURCES  3000  /* "War Resources" == REZDAT.WAR   */
#define WAR_ID_STRINGS    4000  /* "War Strings"   == STRDAT.WAR   */
#define WAR_ID_SOUNDS     5000  /* "War Sounds"    == SFXDAT.SUD   */
#define WAR_ID_MOVIES     6000  /* "War Movies"    == MUDDAT.CUD   */

/* Error codes returned by war_last_error() */
#define WAR_OK          0
#define WAR_ERR_OPEN    1   /* fopen failed */
#define WAR_ERR_FORMAT  2   /* bad magic / truncated header */
#define WAR_ERR_MEMORY  3   /* malloc failed */
#define WAR_ERR_INDEX   4   /* index out of range */
#define WAR_ERR_READ    5   /* short read / corrupt entry */
#define WAR_ERR_FLAGS   6   /* unknown entry flags */

typedef struct WarArchive WarArchive;

/* Open an archive. Keeps the FILE open; reads only the header + offset table
 * (count*4 bytes + count*4 bytes bookkeeping). Returns NULL on error. */
WarArchive *war_open(const char *path);
void war_close(WarArchive *a);

int war_count(const WarArchive *a);       /* number of entries */
int war_id(const WarArchive *a);          /* archive id (1000..6000) */

/* Unpacked size of an entry without loading it.
 * Returns -1 on error, 0 for placeholders. */
long war_entry_size(WarArchive *a, int index);

/* Entry flags byte (0x00 stored, 0x20 compressed, 0xFF placeholder, -1 error). */
int war_entry_flags(WarArchive *a, int index);

/* Load + decompress an entry. Returns a malloc'd buffer (free with war_free)
 * and stores the size in *size (may be NULL). For placeholders and errors
 * returns NULL with *size = 0; check war_last_error() to distinguish. */
unsigned char *war_load(WarArchive *a, int index, unsigned long *size);

/* Same, but into a caller buffer of 'cap' bytes (no allocation).
 * Returns the entry size, 0 for placeholder, -1 on error / too small. */
long war_load_into(WarArchive *a, int index, unsigned char *dst, unsigned long cap);

void war_free(void *p);

int war_last_error(void);

/* Endian-neutral little-endian readers (also used by the gfx decoders). */
#define W2_LE16(p) ((unsigned int)((const unsigned char *)(p))[0] | \
                    ((unsigned int)((const unsigned char *)(p))[1] << 8))
#define W2_LE32(p) ((unsigned long)((const unsigned char *)(p))[0] | \
                    ((unsigned long)((const unsigned char *)(p))[1] << 8) | \
                    ((unsigned long)((const unsigned char *)(p))[2] << 16) | \
                    ((unsigned long)((const unsigned char *)(p))[3] << 24))

#ifdef __cplusplus
}
#endif

#endif /* W2_WAR_H */
