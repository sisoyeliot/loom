#include "internal.h"
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <fnmatch.h>
#include <glob.h>
#include <errno.h>

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
    if (!a || !a[0]) return strdup(b);
    if (!b || !b[0]) return strdup(a);
    size_t la = strlen(a);
    int need_sep = (a[la - 1] != '/');
    char *out = malloc(la + strlen(b) + need_sep + 1);
    sprintf(out, need_sep ? "%s/%s" : "%s%s", a, b);
    return out;
}

const char *loom_path_ext(const char *path) {
    const char *dot = strrchr(path, '.');
    const char *sep = strrchr(path, '/');
    if (!dot || (sep && dot < sep)) return "";
    return dot;
}

const char *loom_path_base(const char *path) {
    const char *s = strrchr(path, '/');
    return s ? s + 1 : path;
}

char *loom_path_dir(const char *path) {
    const char *s = strrchr(path, '/');
    if (!s) return strdup(".");
    return strndup(path, s - path);
}

char *loom_path_noext(const char *path) {
    const char *dot = strrchr(path, '.');
    const char *sep = strrchr(path, '/');
    if (!dot || (sep && dot < sep)) return strdup(path);
    return strndup(path, dot - path);
}

char *loom_path_mangle(const char *path) {
    char *m = strdup(path);
    for (char *p = m; *p; p++) {
        if (*p == '/' || *p == '.' || *p == '-')
            *p = '_';
    }
    return m;
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
    if (stat(path, &st) != 0) return 0;
    return st.st_mtime;
}

static void glob_recurse(const char *dir, const char *pattern, loom_arr_t *out) {
    DIR *d = opendir(dir);
    if (!d) return;
    struct dirent *ent;
    while ((ent = readdir(d))) {
        if (ent->d_name[0] == '.') continue;
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
            if (n > 0 && pattern[n - 1] == '/') n--;
            snprintf(basedir, sizeof(basedir), "%.*s", (int)n, pattern);
        }
        const char *fp = dstar + 2;
        if (*fp == '/') fp++;
        if (!*fp) fp = "*";
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
    if (stat(path, &st) != 0) return 0;
    if (S_ISDIR(st.st_mode)) {
        DIR *d = opendir(path);
        if (!d) return -1;
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
