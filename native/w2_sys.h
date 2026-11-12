/* w2_sys.h - the few OS services AmiW2 needs beyond amiga_gfx / amiga_audio.
 * Free of Amiga headers (game files may include it); the implementation is native/w2_sys.c. */
#ifndef W2_SYS_H
#define W2_SYS_H

#ifdef __cplusplus
extern "C" {
#endif

void w2_sleep_ms(int ms);              /* at least ms milliseconds (AmigaDOS ticks of 20 ms, min. one tick) */
void w2_wait_tof(void);                /* graphics.library WaitTOF: one display frame */
void w2_mkdir(const char *path);       /* create a directory (ignored if it exists) */
int  w2_file_exists(const char *path); /* 1 = a file or directory of that name exists */
int  w2_delete(const char *path);      /* 1 = deleted */
int  w2_has_aga(void);                 /* 1 = AGA chipset (Alice); 0 = OCS/ECS, or MorphOS / no chipset */
int  w2_cpu(void);                     /* 68000/10/20/30/40/60 as 0,10,20,30,40,60 */
unsigned long w2_kickstart(void);      /* exec version << 16 | revision */
/* Calls fn(name, ud) for every plain file in directory path (no subdirectories); returns the count,
 * -1 if the directory cannot be opened. AmigaDOS Lock/Examine/ExNext (libnix readdir is empty on MorphOS). */
int  w2_dir_list(const char *path, void (*fn)(const char *name, void *ud), void *ud);

#ifdef __cplusplus
}
#endif

#endif
