#include "internal.h"
#include <errno.h>

#ifdef _WIN32
char *loom_strndup(const char *s, size_t n) {
  char *result;
  size_t len = 0;
  while (len < n && s[len])
    len++;
  result = malloc(len + 1);
  if (!result)
    return NULL;
  memcpy(result, s, len);
  result[len] = '\0';
  return result;
}

char *loom_strtok_r(char *str, const char *delim, char **saveptr) {
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
#endif

void loom_arr_push(loom_arr_t *a, const char *s) {
  if (a->count >= a->cap) {
    a->cap = a->cap ? a->cap * 2 : 8;
    a->items = realloc(a->items, a->cap * sizeof(char *));
  }
  a->items[a->count++] = strdup(s);
}

void loom_arr_free(loom_arr_t *a) {
  for (int i = 0; i < a->count; i++)
    free(a->items[i]);
  free(a->items);
  a->items = NULL;
  a->count = a->cap = 0;
}

char *loom_path_join(const char *a, const char *b) {
  if (!a || !a[0])
    return strdup(b);
  if (!b || !b[0])
    return strdup(a);
  size_t la = strlen(a);
  int need_sep = (a[la - 1] != '/' && a[la - 1] != '\\');
  char *out = malloc(la + strlen(b) + need_sep + 1);
  sprintf(out, need_sep ? "%s/%s" : "%s%s", a, b);
  return out;
}

const char *loom_path_ext(const char *path) {
  const char *dot = strrchr(path, '.');
  const char *sep1 = strrchr(path, '/');
  const char *sep2 = strrchr(path, '\\');
  const char *sep = sep1 > sep2 ? sep1 : sep2;
  if (!dot || (sep && dot < sep))
    return "";
  return dot;
}

const char *loom_path_base(const char *path) {
  const char *s1 = strrchr(path, '/');
  const char *s2 = strrchr(path, '\\');
  const char *s = s1 > s2 ? s1 : s2;
  return s ? s + 1 : path;
}

char *loom_path_dir(const char *path) {
  const char *s1 = strrchr(path, '/');
  const char *s2 = strrchr(path, '\\');
  const char *s = s1 > s2 ? s1 : s2;
  if (!s)
    return strdup(".");
  return strndup(path, s - path);
}

char *loom_path_noext(const char *path) {
  const char *dot = strrchr(path, '.');
  const char *sep1 = strrchr(path, '/');
  const char *sep2 = strrchr(path, '\\');
  const char *sep = sep1 > sep2 ? sep1 : sep2;
  if (!dot || (sep && dot < sep))
    return strdup(path);
  return strndup(path, dot - path);
}

char *loom_path_mangle(const char *path) {
  char *m = strdup(path);
  for (char *p = m; *p; p++) {
    if (*p == '/' || *p == '\\' || *p == '.' || *p == '-')
      *p = '_';
  }
  return m;
}

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

char *loom_path_getcwd(void) {
  char buf[1024];
  if (GetCurrentDirectoryA(sizeof(buf), buf) > 0)
    return strdup(buf);
  return strdup(".");
}

int loom_mkdir_p(const char *path) {
  char tmp[4096];
  snprintf(tmp, sizeof(tmp), "%s", path);
  for (char *p = tmp + 1; *p; p++) {
    if (*p == '/' || *p == '\\') {
      char old = *p;
      *p = '\0';
      CreateDirectoryA(tmp, NULL);
      *p = old;
    }
  }
  return (CreateDirectoryA(tmp, NULL) || GetLastError() == ERROR_ALREADY_EXISTS)
             ? 0
             : -1;
}

int loom_path_exists(const char *path) {
  return GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
}

time_t loom_path_mtime(const char *path) {
  WIN32_FILE_ATTRIBUTE_DATA attr;
  if (GetFileAttributesExA(path, GetFileExInfoStandard, &attr)) {
    ULARGE_INTEGER ull;
    ull.LowPart = attr.ftLastWriteTime.dwLowDateTime;
    ull.HighPart = attr.ftLastWriteTime.dwHighDateTime;
    return (time_t)(ull.QuadPart / 10000000ULL - 11644473600ULL);
  }
  return 0;
}

static int loom_win_fnmatch(const char *pattern, const char *str) {
  while (*pattern && *str) {
    if (*pattern == '*') {
      while (*pattern == '*')
        pattern++;
      if (!*pattern)
        return 0;
      while (*str) {
        if (loom_win_fnmatch(pattern, str) == 0)
          return 0;
        str++;
      }
      return -1;
    } else if (*pattern == '?' || *pattern == *str) {
      pattern++;
      str++;
    } else {
      return -1;
    }
  }
  while (*pattern == '*')
    pattern++;
  return (*pattern == '\0' && *str == '\0') ? 0 : -1;
}

static void glob_recurse(const char *dir, const char *pattern,
                         loom_arr_t *out) {
  char search[4096];
  snprintf(search, sizeof(search), "%s\\*", dir);
  WIN32_FIND_DATAA fd;
  HANDLE h = FindFirstFileA(search, &fd);
  if (h == INVALID_HANDLE_VALUE)
    return;
  do {
    if (fd.cFileName[0] == '.')
      continue;
    char *full = loom_path_join(dir, fd.cFileName);
    if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      glob_recurse(full, pattern, out);
    } else if (loom_win_fnmatch(pattern, fd.cFileName) == 0) {
      loom_arr_push(out, full);
    }
    free(full);
  } while (FindNextFileA(h, &fd));
  FindClose(h);
}

int loom_glob_match(const char *pattern, loom_arr_t *out) {
  const char *dstar = strstr(pattern, "**");
  if (dstar) {
    char basedir[4096];
    if (dstar == pattern) {
      strcpy(basedir, ".");
    } else {
      size_t n = dstar - pattern;
      if (n > 0 && (pattern[n - 1] == '/' || pattern[n - 1] == '\\'))
        n--;
      snprintf(basedir, sizeof(basedir), "%.*s", (int)n, pattern);
    }
    const char *fp = dstar + 2;
    if (*fp == '/' || *fp == '\\')
      fp++;
    if (!*fp)
      fp = "*";
    glob_recurse(basedir, fp, out);
    return 0;
  }

  char parent[4096];
  const char *last_sep1 = strrchr(pattern, '/');
  const char *last_sep2 = strrchr(pattern, '\\');
  const char *last_sep = last_sep1 > last_sep2 ? last_sep1 : last_sep2;

  if (last_sep) {
    size_t n = last_sep - pattern;
    snprintf(parent, sizeof(parent), "%.*s", (int)n, pattern);
  } else {
    strcpy(parent, ".");
  }
  const char *filename_pat = last_sep ? last_sep + 1 : pattern;

  WIN32_FIND_DATAA fd;
  char search[4096];
  snprintf(search, sizeof(search), "%s\\*", parent);
  HANDLE h = FindFirstFileA(search, &fd);
  if (h != INVALID_HANDLE_VALUE) {
    do {
      if (fd.cFileName[0] == '.')
        continue;
      if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        if (loom_win_fnmatch(filename_pat, fd.cFileName) == 0) {
          char *full = loom_path_join(parent, fd.cFileName);
          loom_arr_push(out, full);
          free(full);
        }
      }
    } while (FindNextFileA(h, &fd));
    FindClose(h);
  }
  return 0;
}

int loom_rmrf(const char *path) {
  DWORD attr = GetFileAttributesA(path);
  if (attr == INVALID_FILE_ATTRIBUTES)
    return 0;

  if (attr & FILE_ATTRIBUTE_DIRECTORY) {
    char search[4096];
    snprintf(search, sizeof(search), "%s\\*", path);
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(search, &fd);
    if (h != INVALID_HANDLE_VALUE) {
      do {
        if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0)
          continue;
        char *child = loom_path_join(path, fd.cFileName);
        loom_rmrf(child);
        free(child);
      } while (FindNextFileA(h, &fd));
      FindClose(h);
    }
    return RemoveDirectoryA(path) ? 0 : -1;
  }
  return DeleteFileA(path) ? 0 : -1;
}

#else

#include <dirent.h>
#include <fnmatch.h>
#include <glob.h>
#include <sys/stat.h>
#include <unistd.h>

char *loom_path_getcwd(void) {
  char buf[1024];
  if (getcwd(buf, sizeof(buf)))
    return strdup(buf);
  return strdup(".");
}

int loom_mkdir_p(const char *path) {
  char tmp[4096];
  snprintf(tmp, sizeof(tmp), "%s", path);
  for (char *p = tmp + 1; *p; p++) {
    if (*p == '/') {
      *p = '\0';
      mkdir(tmp, 0755);
      *p = '/';
    }
  }
  return (mkdir(tmp, 0755) == 0 || errno == EEXIST) ? 0 : -1;
}

int loom_path_exists(const char *path) {
  struct stat st;
  return stat(path, &st) == 0;
}

time_t loom_path_mtime(const char *path) {
  struct stat st;
  if (stat(path, &st) != 0)
    return 0;
  return st.st_mtime;
}

static void glob_recurse(const char *dir, const char *pattern,
                         loom_arr_t *out) {
  DIR *d = opendir(dir);
  if (!d)
    return;
  struct dirent *ent;
  while ((ent = readdir(d))) {
    if (ent->d_name[0] == '.')
      continue;
    char *full = loom_path_join(dir, ent->d_name);
    struct stat st;
    if (stat(full, &st) == 0 && S_ISDIR(st.st_mode)) {
      glob_recurse(full, pattern, out);
    } else if (fnmatch(pattern, ent->d_name, 0) == 0) {
      loom_arr_push(out, full);
    }
    free(full);
  }
  closedir(d);
}

int loom_glob_match(const char *pattern, loom_arr_t *out) {
  const char *dstar = strstr(pattern, "**");
  if (dstar) {
    char basedir[4096];
    if (dstar == pattern) {
      strcpy(basedir, ".");
    } else {
      size_t n = dstar - pattern;
      if (n > 0 && pattern[n - 1] == '/')
        n--;
      snprintf(basedir, sizeof(basedir), "%.*s", (int)n, pattern);
    }
    const char *fp = dstar + 2;
    if (*fp == '/')
      fp++;
    if (!*fp)
      fp = "*";
    glob_recurse(basedir, fp, out);
    return 0;
  }

  glob_t g;
  if (glob(pattern, GLOB_NOSORT, NULL, &g) != 0)
    return -1;
  for (size_t i = 0; i < g.gl_pathc; i++)
    loom_arr_push(out, g.gl_pathv[i]);
  globfree(&g);
  return 0;
}

int loom_rmrf(const char *path) {
  struct stat st;
  if (stat(path, &st) != 0)
    return 0;
  if (S_ISDIR(st.st_mode)) {
    DIR *d = opendir(path);
    if (!d)
      return -1;
    struct dirent *ent;
    while ((ent = readdir(d))) {
      if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
        continue;
      char *child = loom_path_join(path, ent->d_name);
      loom_rmrf(child);
      free(child);
    }
    closedir(d);
    return rmdir(path);
  }
  return unlink(path);
}

#endif
