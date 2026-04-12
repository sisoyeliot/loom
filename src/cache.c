#include "internal.h"
#include <sys/stat.h>

#ifdef _WIN32
static char *loom_strtok_r(char *str, const char *delim, char **saveptr) {
  if (str == NULL)
    str = *saveptr;
  if (str == NULL)
    return NULL;
  str += strspn(str, delim);
  if (*str == '\0') {
    *saveptr = NULL;
    return NULL;
  }
  char *end = str + strcspn(str, delim);
  if (*end == '\0') {
    *saveptr = NULL;
  } else {
    *end = '\0';
    *saveptr = end + 1;
  }
  return str;
}
#define strtok_r loom_strtok_r
#endif

static int read_depfile(const char *path, loom_arr_t *deps) {
  FILE *f = fopen(path, "r");
  if (!f)
    return -1;

  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  fseek(f, 0, SEEK_SET);

  char *buf = malloc(len + 1);
  if (fread(buf, 1, len, f) != (size_t)len) {
    free(buf);
    fclose(f);
    return -1;
  }
  buf[len] = '\0';
  fclose(f);

  char *colon = strchr(buf, ':');
  if (!colon) {
    free(buf);
    return -1;
  }
  char *p = colon + 1;

  char *w = p;
  for (char *r = p; *r; r++) {
    if (*r == '\\' && *(r + 1) == '\n') {
      r++;
      continue;
    }
    *w++ = *r;
  }
  *w = '\0';

  char *save = NULL;
  char *tok = strtok_r(p, " \t\n\r", &save);
  while (tok) {
    if (tok[0])
      loom_arr_push(deps, tok);
    tok = strtok_r(NULL, " \t\n\r", &save);
  }

  free(buf);
  return 0;
}

int loom_needs_rebuild(const char *src, const char *obj, const char *depfile,
                       const char *flags, const char *cachedir) {
  if (!loom_path_exists(obj))
    return 1;

  time_t obj_mt = loom_path_mtime(obj);

  if (loom_path_mtime(src) > obj_mt)
    return 1;

  if (depfile && loom_path_exists(depfile)) {
    loom_arr_t deps = {0};
    if (read_depfile(depfile, &deps) == 0) {
      for (int i = 0; i < deps.count; i++) {
        if (loom_path_mtime(deps.items[i]) > obj_mt) {
          loom_arr_free(&deps);
          return 1;
        }
      }
      loom_arr_free(&deps);
    }
  }

  char *mname = loom_path_mangle(src);
  char *hashfile = loom_path_join(cachedir, mname);
  free(mname);

  loom_hash_t fh;
  loom_sha256(flags, strlen(flags), fh);
  char hex[65];
  loom_hash_hex(fh, hex);

  FILE *f = fopen(hashfile, "r");
  if (!f) {
    free(hashfile);
    return 1;
  }
  char stored[65] = {0};
  if (fread(stored, 1, 64, f) != 64) {
    fclose(f);
    free(hashfile);
    return 1;
  }
  stored[64] = '\0';
  fclose(f);

  int changed = strcmp(hex, stored) != 0;
  free(hashfile);
  return changed;
}

void loom_cache_update(const char *src, const char *flags,
                       const char *cachedir) {
  char *mname = loom_path_mangle(src);
  char *hashfile = loom_path_join(cachedir, mname);
  free(mname);

  loom_hash_t fh;
  loom_sha256(flags, strlen(flags), fh);
  char hex[65];
  loom_hash_hex(fh, hex);

  FILE *f = fopen(hashfile, "w");
  if (f) {
    fwrite(hex, 1, 64, f);
    fclose(f);
  }

  free(hashfile);
}
