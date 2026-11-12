/* hfs.h - minimal read-only Apple Partition Map + HFS (Mac OS Standard) reader for w2conv.
 * Our own implementation from the published Inside Macintosh: Files layout. Big-endian on disk. */
#ifndef W2CONV_HFS_H
#define W2CONV_HFS_H

#include <stdio.h>
#include <stdint.h>

typedef int (*HfsReadFn)(void *ud, int64_t off, void *dst, size_t len);

typedef struct HfsExt { uint32_t start, count; } HfsExt;   /* allocation blocks */

typedef struct HfsFile {
	uint32_t id, parent;
	char     name[32];        /* Mac Roman, 0-terminated */
	char     type[5], creator[5];
	uint32_t data_len, rsrc_len;
	HfsExt   ext[3];          /* first data-fork extents (more in the overflow tree) */
} HfsFile;

typedef struct HfsDir { uint32_t id, parent; char name[32]; } HfsDir;

typedef struct Hfs {
	HfsReadFn rd; void *ud;
	int64_t  part;            /* byte offset of the HFS partition */
	uint32_t blk_size;        /* allocation block size */
	int64_t  blk0;            /* byte offset of allocation block 0 */
	char     volname[32];
	unsigned char *cat; uint32_t cat_len;
	unsigned char *ext; uint32_t ext_len;
	HfsFile *files; int nfiles;
	HfsDir  *dirs;  int ndirs;
} Hfs;

/* Find the HFS volume (via the Apple Partition Map, or bare HFS at offset 1024) and read the catalog.
 * total = size of the readable area in bytes. 0 = ok. */
int  hfs_open(Hfs *h, HfsReadFn rd, void *ud, int64_t total);
void hfs_close(Hfs *h);
/* "Data/War Data" style path below the root, case-insensitive. NULL if not found. */
const HfsFile *hfs_find(const Hfs *h, const char *path);
/* Full path of a file ("Data/War Data") into buf. */
void hfs_path(const Hfs *h, const HfsFile *f, char *buf, size_t cap);
/* Copy the data fork to out. Returns bytes written or -1. */
long hfs_extract(const Hfs *h, const HfsFile *f, FILE *out);

#endif
