/* host_sys.c - the rest of the Amiga platform the game needs on a Linux/WSL host (-DW2_HOST only).
 *
 * AmigaDOS paths. The game opens files as "PROGDIR:data/x.pak" through plain stdio. build-host.sh
 * links with -Wl,--wrap=<fn> for the file functions below, so every such call from the game lands
 * here first and the path is mapped:
 *     PROGDIR:  -> $W2_PROGDIR/   (default: the current directory)
 *     T: RAM:   -> $W2_TMPDIR/    (default: /tmp)
 *     ENV: ENVARC: S: -> $W2_PROGDIR/_<dev>/
 *     other "DEV:" prefixes -> $W2_PROGDIR/_<DEV>/ with a one-time warning.
 * AmigaDOS names are case-insensitive, Linux ones are not: when the mapped path does not exist,
 * each component is matched case-insensitively against the directory (so "Data/FOO.PAK" finds
 * data/foo.pak). A path without a device part ("a/b.txt", "/abs") is left alone apart from that.
 *
 * Also weak host versions of the small native helpers the EU2 layer had (eu2_sys, amiga_uclock,
 * amiga_trap, amiga_startup), so src/ files using them link; a real definition elsewhere wins.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <setjmp.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

unsigned long host_clock_us(void);
void host_clock_advance_ms(unsigned long ms);
void host_logf(const char *fmt, ...);
int  host_map_path(const char *in, char *out, int n);

#define HOST_PATH 1024

/* the libc originals (build-host.sh links with -Wl,--wrap=...); used below so mapping never recurses */
FILE *__real_fopen(const char *path, const char *mode);
FILE *__real_freopen(const char *path, const char *mode, FILE *f);
int   __real_remove(const char *path);
int   __real_unlink(const char *path);
int   __real_rename(const char *a, const char *b);
int   __real_mkdir(const char *path, mode_t m);
int   __real_rmdir(const char *path);
DIR  *__real_opendir(const char *path);
int   __real_stat(const char *path, struct stat *st);
int   __real_access(const char *path, int m);

/* ---- path mapping ------------------------------------------------------------------------- */
static void copy_str(char *d, const char *s, int n)
{
	size_t l = strlen(s);
	if (n <= 0) return;
	if (l >= (size_t)n) l = (size_t)n - 1;
	memcpy(d, s, l);
	d[l] = 0;
}

static const char *env_dir(const char *name, const char *def)
{
	const char *s = getenv(name);
	return (s && *s) ? s : def;
}

/* Case-insensitive resolution: walk the components of `in`, keep each that exists, otherwise look
 * for a case-insensitive match in its directory. Components after the first miss are kept as-is. */
static void resolve_ci(const char *in, char *out, int n)
{
	char buf[HOST_PATH], *comp, *save = NULL;
	int len, missed = 0;
	struct stat st;
	snprintf(buf, sizeof buf, "%s", in);
	if (in[0] == '/') snprintf(out, (size_t)n, "/");
	else out[0] = 0;
	for (comp = strtok_r(buf, "/", &save); comp; comp = strtok_r(NULL, "/", &save)) {
		char cand[HOST_PATH + 512];
		len = (int)strlen(out);
		snprintf(cand, sizeof cand, "%s%s%s", out, (len && out[len - 1] != '/') ? "/" : "", comp);
		if (!missed && __real_stat(cand, &st) != 0) {
			DIR *d = __real_opendir(out[0] ? out : ".");
			struct dirent *e;
			int found = 0;
			if (d) {
				while ((e = readdir(d)) != NULL)
					if (!strcasecmp(e->d_name, comp)) { found = 1; break; }
				if (found)
					snprintf(cand, sizeof cand, "%s%s%s", out, (len && out[len - 1] != '/') ? "/" : "", e->d_name);
				closedir(d);
			}
			if (!found) missed = 1;
		}
		copy_str(out, cand, n);
	}
	if (!out[0]) snprintf(out, (size_t)n, "%s", in);
}

/* Map an AmigaDOS path to a host path. Returns 1 when a device prefix was mapped. */
int host_map_path(const char *in, char *out, int n)
{
	const char *colon, *rest;
	char dev[32], base[HOST_PATH];
	int dl, mapped = 0;
	if (!in) { out[0] = 0; return 0; }
	colon = strchr(in, ':');
	dl = colon ? (int)(colon - in) : 0;
	if (colon && dl > 0 && dl < (int)sizeof dev && !memchr(in, '/', (size_t)dl)) {
		memcpy(dev, in, (size_t)dl); dev[dl] = 0;
		rest = colon + 1;
		if (!strcasecmp(dev, "PROGDIR")) {
			snprintf(base, sizeof base, "%s", env_dir("W2_PROGDIR", "."));
		} else if (!strcasecmp(dev, "T") || !strcasecmp(dev, "RAM")) {
			snprintf(base, sizeof base, "%s", env_dir("W2_TMPDIR", "/tmp"));
		} else {
			static char warned[8][32];
			int i;
			for (i = 0; i < 8 && warned[i][0] && strcasecmp(warned[i], dev); i++) { }
			if (i < 8 && !warned[i][0]) {
				snprintf(warned[i], sizeof warned[i], "%s", dev);
				host_logf("path: device %s: mapped to %s/_%s/", dev, env_dir("W2_PROGDIR", "."), dev);
			}
			snprintf(base, sizeof base, "%s/_%s", env_dir("W2_PROGDIR", "."), dev);
			__real_mkdir(base, 0777);
		}
		snprintf(out, (size_t)n, "%s/%s", base, rest);
		mapped = 1;
	} else {
		snprintf(out, (size_t)n, "%s", in);
	}
	if (__real_access(out, F_OK) != 0) {
		char ci[HOST_PATH];
		resolve_ci(out, ci, sizeof ci);
		copy_str(out, ci, n);
	}
	return mapped;
}

/* ---- --wrap'ed file functions ------------------------------------------------------------- */

FILE *__wrap_fopen(const char *path, const char *mode);
FILE *__wrap_freopen(const char *path, const char *mode, FILE *f);
int   __wrap_remove(const char *path);
int   __wrap_unlink(const char *path);
int   __wrap_rename(const char *a, const char *b);
int   __wrap_mkdir(const char *path, mode_t m);
int   __wrap_rmdir(const char *path);
DIR  *__wrap_opendir(const char *path);
int   __wrap_stat(const char *path, struct stat *st);
int   __wrap_access(const char *path, int m);

static int g_trace = -1;
static void trace(const char *fn, const char *in, const char *out, int ok)
{
	if (g_trace < 0) g_trace = getenv("W2_TRACEFILES") ? 1 : 0;
	if (g_trace) host_logf("%s(%s) -> %s %s", fn, in, out, ok ? "ok" : "FAIL");
}

FILE *__wrap_fopen(const char *path, const char *mode)
{
	char p[HOST_PATH];
	FILE *f;
	if (!path) return __real_fopen(path, mode);
	host_map_path(path, p, sizeof p);
	f = __real_fopen(p, mode);
	trace("fopen", path, p, f != NULL);
	return f;
}

FILE *__wrap_freopen(const char *path, const char *mode, FILE *f)
{
	char p[HOST_PATH];
	if (!path) return __real_freopen(path, mode, f);
	if ((f == stdout || f == stderr) && getenv("W2_STDOUT")) {   /* keep the log on the terminal */
		host_logf("freopen(%s) of std%s skipped (W2_STDOUT)", path, f == stdout ? "out" : "err");
		return f;
	}
	host_map_path(path, p, sizeof p);
	return __real_freopen(p, mode, f);
}

#define WRAP1(ret, fn, ...) \
	char p[HOST_PATH]; ret r; host_map_path(path, p, sizeof p); r = __real_##fn(p, ##__VA_ARGS__); \
	trace(#fn, path, p, r == 0); return r;

int __wrap_remove(const char *path) { WRAP1(int, remove) }
int __wrap_unlink(const char *path) { WRAP1(int, unlink) }
int __wrap_mkdir(const char *path, mode_t m) { WRAP1(int, mkdir, m) }
int __wrap_rmdir(const char *path) { WRAP1(int, rmdir) }
int __wrap_stat(const char *path, struct stat *st) { WRAP1(int, stat, st) }
int __wrap_access(const char *path, int m) { WRAP1(int, access, m) }

int __wrap_rename(const char *a, const char *b)
{
	char pa[HOST_PATH], pb[HOST_PATH];
	host_map_path(a, pa, sizeof pa);
	host_map_path(b, pb, sizeof pb);
	return __real_rename(pa, pb);
}

DIR *__wrap_opendir(const char *path)
{
	char p[HOST_PATH];
	host_map_path(path, p, sizeof p);
	return __real_opendir(p);
}

/* ---- weak host versions of the old native helpers ----------------------------------------- */
#define WEAK __attribute__((weak))

/* w2_sys.h (native/w2_sys.c is Amiga-only). WaitTOF = one PAL/NTSC frame of (virtual) time. */
WEAK void w2_sleep_ms(int ms) { host_clock_advance_ms((unsigned long)(ms > 20 ? ms : 20)); }
WEAK void w2_wait_tof(void) { host_clock_advance_ms(getenv("W2_NTSC") ? 17 : 20); }
WEAK void w2_mkdir(const char *path) { mkdir(path, 0777); }               /* mapped by the wrapper */
WEAK int  w2_file_exists(const char *path) { return access(path, F_OK) == 0; }
WEAK int  w2_delete(const char *path) { return remove(path) == 0; }
WEAK int  w2_dir_list(const char *path, void (*fn)(const char *name, void *ud), void *ud)
{
	DIR *d = opendir(path);       /* wrapped: PROGDIR: is mapped */
	struct dirent *de;
	int n = 0;
	if (!d || !fn) { if (d) closedir(d); return -1; }
	while ((de = readdir(d)) != NULL) {
		if (de->d_name[0] == '.') continue;
		fn(de->d_name, ud); n++;
	}
	closedir(d);
	return n;
}
WEAK int  w2_has_aga(void) { const char *s = getenv("W2_AGA"); return s ? atoi(s) != 0 : 1; }
WEAK int  w2_cpu(void) { const char *s = getenv("W2_CPU"); return s ? atoi(s) : 60; }
WEAK unsigned long w2_kickstart(void) { return (40UL << 16) | 68UL; }

/* Entry: on the Amiga native/amiga_stack.c calls w2_main() on a big stack. Weak, so a main of its own
 * (tools/host_selftest.c) wins; w2_main is weak-referenced so the self-test links without it. */
extern int w2_main(void) __attribute__((weak));
WEAK int main(void)
{
	if (!w2_main) { fprintf(stderr, "host: no w2_main linked\n"); return 20; }
	return w2_main();
}

/* eu2_sys.h */
WEAK void eu2_sleep_ms(int ms) { host_clock_advance_ms((unsigned long)(ms > 0 ? ms : 1)); }
WEAK void eu2_mkdir(const char *path) { mkdir(path, 0777); }   /* mapped by the wrapper */

/* amiga_uclock.h */
WEAK unsigned long amiga_uclock_us(void) { return host_clock_us(); }
WEAK unsigned long amiga_uclock_freq(void) { return 1000000UL; }

/* amiga_trap.h - no CPU trap catching on the host; ASan/UBSan report instead */
WEAK jmp_buf amiga_trap_jb;
WEAK volatile unsigned long amiga_trap_number, amiga_trap_pc, amiga_trap_sr, amiga_trap_fmt, amiga_trap_regs[16];
WEAK void amiga_trap_install(void) { }
WEAK void amiga_trap_disarm(void) { }
WEAK void amiga_trap_describe(char *buf, int len) { if (buf && len > 0) snprintf(buf, (size_t)len, "no trap (host)"); }

/* amiga_startup.h */
WEAK int  amigastartup_ask_backend(int rtg_available) { return rtg_available ? 1 : 0; }
WEAK void amigastartup_error(const char *text) { host_logf("startup error: %s", text ? text : "(null)"); }
WEAK int  amigastartup_mixcost(void) { return 0; }
WEAK int  amigastartup_cpu(void) { return 68060; }
WEAK void amiga_select_backend(int argc, char **argv) { (void)argc; (void)argv; }
WEAK void SDLmini_Log(const char *msg) { host_logf("%s", msg ? msg : "(null)"); }

/* ASan defaults: LeakSanitizer does not work under WSL1 (no ptrace); ASAN_OPTIONS still overrides. */
const char *__asan_default_options(void);
const char *__asan_default_options(void) { return "detect_leaks=0:abort_on_error=0:print_summary=1"; }
const char *__ubsan_default_options(void);
const char *__ubsan_default_options(void) { return "print_stacktrace=1"; }
