#ifndef LOOM_INTERNAL_H
#define LOOM_INTERNAL_H

#define LOOM_NO_SHORT_NAMES
#include "loom.h"

#ifdef _WIN32
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#ifndef _CRT_NONSTDC_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS
#endif
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define LOOM_ERR(fmt, ...)                                                     \
  fprintf(stderr, "\033[1;31merror:\033[0m " fmt "\n", ##__VA_ARGS__)
#define LOOM_WARN(fmt, ...)                                                    \
  fprintf(stderr, "\033[1;33mwarn:\033[0m " fmt "\n", ##__VA_ARGS__)
#define LOOM_INFO(fmt, ...)                                                    \
  fprintf(stderr, "\033[1;36minfo:\033[0m " fmt "\n", ##__VA_ARGS__)
#define LOOM_OK(fmt, ...)                                                      \
  fprintf(stderr, "\033[1;32m  ok:\033[0m " fmt "\n", ##__VA_ARGS__)

void loom_arr_push(loom_arr_t *a, const char *s);
void loom_arr_free(loom_arr_t *a);

int loom_detect_toolchain(loom_toolchain_t *tc);

typedef uint8_t loom_hash_t[32];
void loom_sha256(const void *data, size_t len, loom_hash_t out);
int loom_sha256_file(const char *path, loom_hash_t out);
void loom_hash_hex(const loom_hash_t h, char *out);

int loom_needs_rebuild(const char *src, const char *obj, const char *depfile,
                       const char *flags, const char *cachedir);
void loom_cache_update(const char *src, const char *flags,
                       const char *cachedir);

typedef struct {
  const char *label;
  char **argv;
} loom_job_t;

int loom_exec(char **argv);
int loom_exec_capture(char **argv, char *buf, size_t bufsz);
int loom_exec_jobs(loom_job_t *jobs, int n, int total, int maxj, int verbose);
int loom_nproc(void);

char *loom_path_join(const char *a, const char *b);
const char *loom_path_ext(const char *path);
const char *loom_path_base(const char *path);
char *loom_path_dir(const char *path);
char *loom_path_noext(const char *path);
char *loom_path_mangle(const char *path);
int loom_mkdir_p(const char *path);
char *loom_path_getcwd(void);
int loom_path_exists(const char *path);
time_t loom_path_mtime(const char *path);
int loom_glob_match(const char *pattern, loom_arr_t *out);
int loom_rmrf(const char *path);

loom_build_t *loom_setup(int argc, char **argv);
int loom_run_build(loom_build_t *b);
void loom_teardown(loom_build_t *b);

int loom_cmd_init(const char *dir);
int loom_cmd_create(const char *name);

#endif
