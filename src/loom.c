/*
 * loom.c — Core implementation of the Loom build system.
 *
 * Implements all public API functions declared in loom.h together with
 * the internal build-execution engine.  The runner binary (produced by
 * compiling the user's build.c against libloom.a) calls loom_setup(),
 * lets the user populate targets via the API, then calls
 * loom_run_build() which drives compilation, linking and installation.
 */

#include "internal.h"

static void arr_push_unique(loom_arr_t *a, const char *s) {
  for (int i = 0; i < a->count; i++)
    if (strcmp(a->items[i], s) == 0)
      return;
  loom_arr_push(a, s);
}

/* ---- target creation -------------------------------------------- */

static loom_target_t *target_new(loom_build_t *b, const char *name,
                                 loom_target_type_t type) {
  if (b->n_targets >= b->cap_targets) {
    b->cap_targets = b->cap_targets ? b->cap_targets * 2 : 4;
    b->targets = realloc(b->targets, b->cap_targets * sizeof(loom_target_t *));
  }
  loom_target_t *t = calloc(1, sizeof(*t));
  t->name = strdup(name);
  t->type = type;
  t->std = LOOM_C11;
  t->opt = LOOM_OPT_NONE;
  b->targets[b->n_targets++] = t;
  return t;
}

loom_target_t *loom_add_executable(loom_build_t *b, const char *name) {
  return target_new(b, name, LOOM_EXECUTABLE);
}

loom_target_t *loom_add_static_library(loom_build_t *b, const char *name) {
  return target_new(b, name, LOOM_STATIC_LIB);
}

loom_target_t *loom_add_shared_library(loom_build_t *b, const char *name) {
  return target_new(b, name, LOOM_SHARED_LIB);
}

/* ---- target configuration --------------------------------------- */

void loom_add_source(loom_target_t *t, const char *path) {
  arr_push_unique(&t->sources, path);
}

void loom_add_sources(loom_target_t *t, const char *pattern) {
  loom_arr_t m = {0};
  loom_glob_match(pattern, &m);
  for (int i = 0; i < m.count; i++)
    arr_push_unique(&t->sources, m.items[i]);
  loom_arr_free(&m);
}

void loom_add_include_dir(loom_target_t *t, const char *dir) {
  arr_push_unique(&t->includes, dir);
}

void loom_add_cflag(loom_target_t *t, const char *flag) {
  loom_arr_push(&t->cflags, flag);
}

void loom_add_ldflag(loom_target_t *t, const char *flag) {
  loom_arr_push(&t->ldflags, flag);
}

void loom_link_library(loom_target_t *t, const char *lib) {
  loom_arr_push(&t->libs, lib);
}

void loom_link_system_library(loom_target_t *t, const char *name) {
  char cmd[512], buf[2048];

  snprintf(cmd, sizeof(cmd), "pkg-config --cflags %s", name);
  char *av1[] = {"sh", "-c", cmd, NULL};
  if (loom_exec_capture(av1, buf, sizeof(buf)) == 0 && buf[0]) {
    char *sv = NULL;
    char *tok = strtok_r(buf, " \t\n\r", &sv);
    while (tok) {
      if (tok[0])
        loom_add_cflag(t, tok);
      tok = strtok_r(NULL, " \t\n\r", &sv);
    }
  }

  snprintf(cmd, sizeof(cmd), "pkg-config --libs %s", name);
  char *av2[] = {"sh", "-c", cmd, NULL};
  if (loom_exec_capture(av2, buf, sizeof(buf)) == 0 && buf[0]) {
    char *sv = NULL;
    char *tok = strtok_r(buf, " \t\n\r", &sv);
    while (tok) {
      if (tok[0])
        loom_add_ldflag(t, tok);
      tok = strtok_r(NULL, " \t\n\r", &sv);
    }
    return;
  }

  char flag[256];
  snprintf(flag, sizeof(flag), "-l%s", name);
  loom_add_ldflag(t, flag);
}

void loom_set_standard(loom_target_t *t, loom_std_t s) { t->std = s; }
void loom_set_optimization(loom_target_t *t, loom_opt_t o) { t->opt = o; }
void loom_set_test(loom_target_t *t) { t->test = 1; }
void loom_install_artifact(loom_target_t *t) { t->do_install = 1; }

void loom_set_output_dir(loom_target_t *t, const char *dir) {
  free(t->outdir);
  t->outdir = strdup(dir);
}

void loom_set_output_name(loom_target_t *t, const char *name) {
  free(t->outname);
  t->outname = strdup(name);
}

/* ---- build context ---------------------------------------------- */

const loom_toolchain_t *loom_get_toolchain(loom_build_t *b) { return &b->tc; }
loom_os_t loom_get_os(loom_build_t *b) { return b->tc.os; }
loom_arch_t loom_get_arch(loom_build_t *b) { return b->tc.arch; }

void loom_set_install_prefix(loom_build_t *b, const char *prefix) {
  free(b->prefix);
  b->prefix = strdup(prefix);
}

int loom_option_bool(loom_build_t *b, const char *name, int def) {
  for (int i = 0; i < b->n_opts; i++)
    if (strcmp(b->opts[i].key, name) == 0)
      return strcmp(b->opts[i].val, "true") == 0 ||
             strcmp(b->opts[i].val, "1") == 0;
  return def;
}

const char *loom_option_str(loom_build_t *b, const char *name,
                            const char *def) {
  for (int i = 0; i < b->n_opts; i++)
    if (strcmp(b->opts[i].key, name) == 0)
      return b->opts[i].val;
  return def;
}

void loom_install_header(loom_build_t *b, const char *path) {
  loom_arr_push(&b->install_hdrs, path);
}

/* ---- custom steps ----------------------------------------------- */

loom_step_t *loom_add_step(loom_build_t *b, const char *name,
                           void (*fn)(loom_step_t *)) {
  if (b->n_steps >= b->cap_steps) {
    b->cap_steps = b->cap_steps ? b->cap_steps * 2 : 4;
    b->steps = realloc(b->steps, b->cap_steps * sizeof(loom_step_t *));
  }
  loom_step_t *s = calloc(1, sizeof(*s));
  s->name = strdup(name);
  s->fn = fn;
  s->idx = b->n_steps;
  b->steps[b->n_steps++] = s;
  return s;
}

void loom_step_depends_on(loom_step_t *step, loom_step_t *dep) {
  if (step->n_deps >= step->cap_deps) {
    step->cap_deps = step->cap_deps ? step->cap_deps * 2 : 4;
    step->deps = realloc(step->deps, step->cap_deps * sizeof(int));
  }
  step->deps[step->n_deps++] = dep->idx;
}

void loom_step_set_userdata(loom_step_t *s, void *d) { s->udata = d; }
void *loom_step_get_userdata(loom_step_t *s) { return s->udata; }

/* ================================================================= */
/*  Build-execution engine                                           */
/* ================================================================= */

static const char *std_flag(loom_std_t s) {
  switch (s) {
  case LOOM_C89:
    return "-std=c89";
  case LOOM_C99:
    return "-std=c99";
  case LOOM_C11:
    return "-std=c11";
  case LOOM_C17:
    return "-std=c17";
  case LOOM_C23:
    return "-std=c23";
  }
  return "-std=c11";
}

static const char *opt_flags(loom_opt_t o) {
  switch (o) {
  case LOOM_OPT_NONE:
    return "-O0";
  case LOOM_OPT_DEBUG:
    return "-Og -g";
  case LOOM_OPT_RELEASE_SAFE:
    return "-O2 -g";
  case LOOM_OPT_RELEASE_FAST:
    return "-O3";
  case LOOM_OPT_RELEASE_SMALL:
    return "-Os";
  }
  return "-O0";
}

static char *build_flags_key(loom_build_t *b, loom_target_t *t) {
  size_t cap = 8192;
  char *buf = malloc(cap);
  int off = 0;

  off += snprintf(buf + off, cap - off, "%s %s %s", b->tc.cc, std_flag(t->std),
                  opt_flags(t->opt));
  for (int i = 0; i < t->includes.count; i++)
    off += snprintf(buf + off, cap - off, " -I%s", t->includes.items[i]);
  for (int i = 0; i < t->cflags.count; i++)
    off += snprintf(buf + off, cap - off, " %s", t->cflags.items[i]);
  if (b->release && t->opt == LOOM_OPT_NONE)
    off += snprintf(buf + off, cap - off, " -O2 -DNDEBUG");

  return buf;
}

static char *src_to_obj(loom_build_t *b, loom_target_t *t, const char *src) {
  char *mangled = loom_path_mangle(src);
  char *objbase = loom_path_join(b->cachedir, "obj");
  char *objdir = loom_path_join(objbase, t->name);
  size_t len = strlen(objdir) + strlen(mangled) + 4;
  char *obj = malloc(len);
  snprintf(obj, len, "%s/%s.o", objdir, mangled);
  free(mangled);
  free(objbase);
  free(objdir);
  return obj;
}

static char *src_to_dep(loom_build_t *b, loom_target_t *t, const char *src) {
  char *mangled = loom_path_mangle(src);
  char *depbase = loom_path_join(b->cachedir, "dep");
  char *depdir = loom_path_join(depbase, t->name);
  size_t len = strlen(depdir) + strlen(mangled) + 4;
  char *dep = malloc(len);
  snprintf(dep, len, "%s/%s.d", depdir, mangled);
  free(mangled);
  free(depbase);
  free(depdir);
  return dep;
}

#define ARGV_PUSH(s)                                                           \
  do {                                                                         \
    if (ac >= cap - 1) {                                                       \
      cap *= 2;                                                                \
      av = realloc(av, cap * sizeof(char *));                                  \
    }                                                                          \
    av[ac++] = strdup(s);                                                      \
  } while (0)

static char **make_compile_argv(loom_build_t *b, loom_target_t *t,
                                const char *src, const char *obj,
                                const char *dep) {
  int ac = 0, cap = 32;
  char **av = calloc(cap, sizeof(char *));

  ARGV_PUSH(b->tc.cc);
  ARGV_PUSH(std_flag(t->std));

  if (b->release && t->opt == LOOM_OPT_NONE) {
    ARGV_PUSH("-O2");
    ARGV_PUSH("-DNDEBUG");
  } else {
    char *tmp = strdup(opt_flags(t->opt));
    char *sv = NULL, *tok = strtok_r(tmp, " ", &sv);
    while (tok) {
      ARGV_PUSH(tok);
      tok = strtok_r(NULL, " ", &sv);
    }
    free(tmp);
  }

  if (t->type == LOOM_SHARED_LIB)
    ARGV_PUSH("-fPIC");

  for (int i = 0; i < t->includes.count; i++) {
    char inc[4096];
    snprintf(inc, sizeof(inc), "-I%s", t->includes.items[i]);
    ARGV_PUSH(inc);
  }
  for (int i = 0; i < t->cflags.count; i++)
    ARGV_PUSH(t->cflags.items[i]);

  ARGV_PUSH("-MD");
  ARGV_PUSH("-MF");
  ARGV_PUSH(dep);
  ARGV_PUSH("-c");
  ARGV_PUSH(src);
  ARGV_PUSH("-o");
  ARGV_PUSH(obj);
  av[ac] = NULL;
  return av;
}

static char **make_link_argv(loom_build_t *b, loom_target_t *t, char **objs,
                             int n_objs, const char *out) {
  int ac = 0, cap = 64;
  char **av = calloc(cap, sizeof(char *));

  if (t->type == LOOM_STATIC_LIB) {
    ARGV_PUSH(b->tc.ar);
    ARGV_PUSH("rcs");
    ARGV_PUSH(out);
    for (int i = 0; i < n_objs; i++)
      ARGV_PUSH(objs[i]);
  } else {
    ARGV_PUSH(b->tc.cc);
    if (t->type == LOOM_SHARED_LIB)
      ARGV_PUSH("-shared");
    for (int i = 0; i < n_objs; i++)
      ARGV_PUSH(objs[i]);
    ARGV_PUSH("-o");
    ARGV_PUSH(out);
    for (int i = 0; i < t->ldflags.count; i++)
      ARGV_PUSH(t->ldflags.items[i]);
    for (int i = 0; i < t->libs.count; i++) {
      char l[256];
      snprintf(l, sizeof(l), "-l%s", t->libs.items[i]);
      ARGV_PUSH(l);
    }
  }

  av[ac] = NULL;
  return av;
}

#undef ARGV_PUSH

static void free_argv(char **av) {
  if (!av)
    return;
  for (int i = 0; av[i]; i++)
    free(av[i]);
  free(av);
}

static const char *output_name(loom_target_t *t) {
  static char buf[512];
  const char *base = t->outname ? t->outname : t->name;
  switch (t->type) {
  case LOOM_EXECUTABLE:
#if defined(_WIN32) || defined(__CYGWIN__) || defined(__MSYS__)
    snprintf(buf, sizeof(buf), "%s.exe", base);
#else
    snprintf(buf, sizeof(buf), "%s", base);
#endif
    break;
  case LOOM_STATIC_LIB:
    snprintf(buf, sizeof(buf), "lib%s.a", base);
    break;
  case LOOM_SHARED_LIB:
#ifdef __APPLE__
    snprintf(buf, sizeof(buf), "lib%s.dylib", base);
#else
    snprintf(buf, sizeof(buf), "lib%s.so", base);
#endif
    break;
  }
  return buf;
}

static int build_target(loom_build_t *b, loom_target_t *t) {
  if (t->sources.count == 0) {
    LOOM_ERR("target '%s' has no source files", t->name);
    return 1;
  }

  char *obj_base = loom_path_join(b->cachedir, "obj");
  char *obj_dir = loom_path_join(obj_base, t->name);
  char *dep_base = loom_path_join(b->cachedir, "dep");
  char *dep_dir = loom_path_join(dep_base, t->name);
  char *cch_base = loom_path_join(b->cachedir, "cache");
  char *cch_dir = loom_path_join(cch_base, t->name);
  loom_mkdir_p(obj_dir);
  loom_mkdir_p(dep_dir);
  loom_mkdir_p(cch_dir);
  free(obj_base);
  free(dep_base);
  free(cch_base);

  char *flagskey = build_flags_key(b, t);

  int nsrc = t->sources.count;
  char **obj_paths = calloc(nsrc, sizeof(char *));
  loom_job_t *cjobs = calloc(nsrc, sizeof(loom_job_t));
  char **clabels = calloc(nsrc, sizeof(char *));
  int nc = 0, any_new = 0;

  for (int i = 0; i < nsrc; i++) {
    const char *src = t->sources.items[i];
    obj_paths[i] = src_to_obj(b, t, src);
    char *dep = src_to_dep(b, t, src);

    if (loom_needs_rebuild(src, obj_paths[i], dep, flagskey, cch_dir)) {
      char lbl[512];
      snprintf(lbl, sizeof(lbl), "CC %s", src);
      clabels[nc] = strdup(lbl);
      cjobs[nc].label = clabels[nc];
      cjobs[nc].argv = make_compile_argv(b, t, src, obj_paths[i], dep);
      nc++;
      any_new = 1;
    }
    free(dep);
  }

  const char *oname = output_name(t);
  char *outpath = t->outdir ? loom_path_join(t->outdir, oname) : strdup(oname);
  int need_link = any_new || !loom_path_exists(outpath);
  int total = nc + (need_link ? 1 : 0);
  int rc = 0;

  if (nc > 0) {
    rc = loom_exec_jobs(cjobs, nc, total, b->jobs, b->verbose);
    if (rc != 0)
      goto done;
    for (int i = 0; i < nsrc; i++)
      loom_cache_update(t->sources.items[i], flagskey, cch_dir);
  }

  if (need_link) {
    const char *verb = t->type == LOOM_STATIC_LIB ? "AR" : "LINK";
    fprintf(stderr, "\033[1m[%d/%d]\033[0m %s %s\n", total, total, verb, oname);

    if (t->outdir)
      loom_mkdir_p(t->outdir);
    char **lav = make_link_argv(b, t, obj_paths, nsrc, outpath);
    if (b->verbose) {
      fprintf(stderr, " ");
      for (int k = 0; lav[k]; k++)
        fprintf(stderr, " %s", lav[k]);
      fprintf(stderr, "\n");
    }
    rc = loom_exec(lav);
    free_argv(lav);
    if (rc != 0) {
      LOOM_ERR("linking failed for '%s'", t->name);
      goto done;
    }
  }

  if (total > 0)
    LOOM_OK("built %s", outpath);
  else
    fprintf(stderr, "\033[1;32m%s\033[0m is up to date\n", t->name);

done:
  for (int i = 0; i < nc; i++) {
    free(clabels[i]);
    free_argv(cjobs[i].argv);
  }
  free(cjobs);
  free(clabels);
  for (int i = 0; i < nsrc; i++)
    free(obj_paths[i]);
  free(obj_paths);
  free(flagskey);
  free(obj_dir);
  free(dep_dir);
  free(cch_dir);
  free(outpath);
  return rc;
}

static void run_steps(loom_build_t *b) {
  int changed = 1;
  while (changed) {
    changed = 0;
    for (int i = 0; i < b->n_steps; i++) {
      loom_step_t *s = b->steps[i];
      if (s->done)
        continue;
      int ready = 1;
      for (int j = 0; j < s->n_deps; j++)
        if (!b->steps[s->deps[j]]->done) {
          ready = 0;
          break;
        }
      if (ready) {
        LOOM_INFO("step: %s", s->name);
        s->fn(s);
        s->done = 1;
        changed = 1;
      }
    }
  }
}

static int do_query(loom_build_t *b) {
  const loom_toolchain_t *tc = &b->tc;

  const char *cc_str = "unknown";
  if (tc->type == LOOM_CC_GCC)
    cc_str = "gcc";
  if (tc->type == LOOM_CC_CLANG)
    cc_str = "clang";
  if (tc->type == LOOM_CC_TCC)
    cc_str = "tcc";

  const char *os_str = "unknown";
  if (tc->os == LOOM_OS_LINUX)
    os_str = "linux";
  if (tc->os == LOOM_OS_MACOS)
    os_str = "macos";
  if (tc->os == LOOM_OS_FREEBSD)
    os_str = "freebsd";
  if (tc->os == LOOM_OS_WINDOWS)
    os_str = "windows";

  const char *ar_str = "unknown";
  if (tc->arch == LOOM_ARCH_X86_64)
    ar_str = "x86_64";
  if (tc->arch == LOOM_ARCH_AARCH64)
    ar_str = "aarch64";
  if (tc->arch == LOOM_ARCH_X86)
    ar_str = "x86";
  if (tc->arch == LOOM_ARCH_ARM)
    ar_str = "arm";
  if (tc->arch == LOOM_ARCH_RISCV64)
    ar_str = "riscv64";

  printf("toolchain:\n");
  printf("  compiler : %s (%s %d.%d.%d)\n", tc->cc, cc_str, tc->ver_major,
         tc->ver_minor, tc->ver_patch);
  printf("  archiver : %s\n", tc->ar);
  printf("  os       : %s\n", os_str);
  printf("  arch     : %s\n", ar_str);
  printf("  jobs     : %d\n", b->jobs);
  printf("\ntargets:\n");
  for (int i = 0; i < b->n_targets; i++) {
    loom_target_t *t = b->targets[i];
    const char *k = "exe";
    if (t->type == LOOM_STATIC_LIB)
      k = "static";
    if (t->type == LOOM_SHARED_LIB)
      k = "shared";
    printf("  %s (%s, %d sources%s)\n", t->name, k, t->sources.count,
           t->test ? ", test" : "");
  }
  return 0;
}

static int do_run(loom_build_t *b) {
  for (int i = 0; i < b->n_targets; i++) {
    loom_target_t *t = b->targets[i];
    if (t->type != LOOM_EXECUTABLE || t->test)
      continue;
    const char *name = output_name(t);
    char path[4096];
    if (t->outdir)
      snprintf(path, sizeof(path), "%s/%s", t->outdir, name);
    else
      snprintf(path, sizeof(path), "./%s", name);

    printf("\n");

    int n = 1;
    char **rav = calloc(b->argc + 2, sizeof(char *));
    rav[0] = path;
    int past = 0;
    for (int j = 0; j < b->argc; j++) {
      if (strcmp(b->argv[j], "--") == 0) {
        past = 1;
        continue;
      }
      if (past)
        rav[n++] = b->argv[j];
    }
    rav[n] = NULL;
    int rc = loom_exec(rav);
    free(rav);
    return rc;
  }
  LOOM_ERR("no executable target found");
  return 1;
}

static int do_test(loom_build_t *b) {
  int passed = 0, failed = 0;

  for (int i = 0; i < b->n_targets; i++) {
    loom_target_t *t = b->targets[i];
    if (!t->test || t->type != LOOM_EXECUTABLE)
      continue;

    const char *name = output_name(t);
    char path[4096];
    if (t->outdir)
      snprintf(path, sizeof(path), "%s/%s", t->outdir, name);
    else
      snprintf(path, sizeof(path), "./%s", name);

    fprintf(stderr, "\n\033[1mtest: %s\033[0m\n", t->name);
    char *rav[] = {path, NULL};
    int rc = loom_exec(rav);

    if (rc == 0) {
      fprintf(stderr, "\033[1;32m  PASS\033[0m %s\n", t->name);
      passed++;
    } else {
      fprintf(stderr, "\033[1;31m  FAIL\033[0m %s (exit %d)\n", t->name, rc);
      failed++;
    }
  }

  fprintf(stderr, "\n%d passed, %d failed\n", passed, failed);
  return failed > 0 ? 1 : 0;
}

static int do_install(loom_build_t *b) {
  int any = 0;

  for (int i = 0; i < b->n_targets; i++) {
    loom_target_t *t = b->targets[i];
    if (!t->do_install)
      continue;
    any = 1;

    const char *sub = t->type == LOOM_EXECUTABLE ? "bin" : "lib";
    char *dst_dir = loom_path_join(b->prefix, sub);
    loom_mkdir_p(dst_dir);

    const char *name = output_name(t);
    char *src_path = t->outdir ? loom_path_join(t->outdir, name) : strdup(name);
    char *dst_path = loom_path_join(dst_dir, name);

    char cmd[8192];
    snprintf(cmd, sizeof(cmd), "install -m %s '%s' '%s'",
             t->type == LOOM_EXECUTABLE ? "755" : "644", src_path, dst_path);
    LOOM_INFO("install %s -> %s", name, dst_path);
    int rc = system(cmd);

    free(dst_dir);
    free(src_path);
    free(dst_path);
    if (rc != 0)
      return 1;
  }

  for (int i = 0; i < b->install_hdrs.count; i++) {
    const char *hdr = b->install_hdrs.items[i];
    const char *base = loom_path_base(hdr);
    char *incdir = loom_path_join(b->prefix, "include");
    char *dst = loom_path_join(incdir, base);
    loom_mkdir_p(incdir);

    char cmd[8192];
    snprintf(cmd, sizeof(cmd), "install -m 644 '%s' '%s'", hdr, dst);
    LOOM_INFO("install %s -> %s", base, dst);
    int rc = system(cmd);

    free(incdir);
    free(dst);
    if (rc != 0)
      return 1;
    any = 1;
  }

  if (!any)
    LOOM_WARN("no targets marked for installation");
  return 0;
}

/* ================================================================= */
/*  Public: context lifecycle                                        */
/* ================================================================= */

loom_build_t *loom_setup(int argc, char **argv) {
  loom_build_t *b = calloc(1, sizeof(*b));
  b->argc = argc;
  b->argv = argv;
  b->jobs = loom_nproc();
  b->prefix = strdup("/usr/local");
  b->cachedir = strdup(".loom");
  b->action = "build";

  b->root = loom_path_getcwd();

  for (int i = 0; i < argc; i++) {
    const char *a = argv[i];
    if (strcmp(a, "build") == 0)
      b->action = "build";
    else if (strcmp(a, "run") == 0)
      b->action = "run";
    else if (strcmp(a, "test") == 0)
      b->action = "test";
    else if (strcmp(a, "install") == 0)
      b->action = "install";
    else if (strcmp(a, "query") == 0)
      b->action = "query";
    else if (strcmp(a, "-v") == 0 || strcmp(a, "--verbose") == 0)
      b->verbose = 1;
    else if (strcmp(a, "--release") == 0)
      b->release = 1;
    else if (strncmp(a, "-j", 2) == 0) {
      const char *num = a + 2;
      if (!*num && i + 1 < argc)
        num = argv[++i];
      if (*num)
        b->jobs = atoi(num);
    } else if (strncmp(a, "-D", 2) == 0) {
      const char *kv = a + 2;
      char *eq = strchr(kv, '=');
      if (eq) {
        if (b->n_opts >= b->cap_opts) {
          b->cap_opts = b->cap_opts ? b->cap_opts * 2 : 8;
          b->opts = realloc(b->opts, b->cap_opts * sizeof(loom_kv_t));
        }
        b->opts[b->n_opts].key = strndup(kv, eq - kv);
        b->opts[b->n_opts].val = strdup(eq + 1);
        b->n_opts++;
      }
    }
  }

  if (loom_detect_toolchain(&b->tc) != 0) {
    LOOM_ERR("no C compiler found");
    free(b);
    return NULL;
  }

  return b;
}

int loom_run_build(loom_build_t *b) {
  if (strcmp(b->action, "query") == 0)
    return do_query(b);

  loom_mkdir_p(b->cachedir);

  int any = 0;
  for (int i = 0; i < b->n_targets; i++) {
    loom_target_t *t = b->targets[i];
    if (t->test && strcmp(b->action, "test") != 0)
      continue;
    if (!t->test && strcmp(b->action, "test") == 0)
      continue;

    int rc = build_target(b, t);
    if (rc != 0)
      return rc;
    any = 1;
  }

  if (!any && strcmp(b->action, "test") == 0)
    LOOM_WARN("no test targets defined");

  run_steps(b);

  if (strcmp(b->action, "run") == 0)
    return do_run(b);
  if (strcmp(b->action, "test") == 0)
    return do_test(b);
  if (strcmp(b->action, "install") == 0)
    return do_install(b);

  return 0;
}

void loom_teardown(loom_build_t *b) {
  for (int i = 0; i < b->n_targets; i++) {
    loom_target_t *t = b->targets[i];
    free(t->name);
    free(t->outname);
    loom_arr_free(&t->sources);
    loom_arr_free(&t->includes);
    loom_arr_free(&t->cflags);
    loom_arr_free(&t->ldflags);
    loom_arr_free(&t->libs);
    loom_arr_free(&t->syslibs);
    free(t->outdir);
    free(t);
  }
  free(b->targets);

  for (int i = 0; i < b->n_steps; i++) {
    free(b->steps[i]->name);
    free(b->steps[i]->deps);
    free(b->steps[i]);
  }
  free(b->steps);

  for (int i = 0; i < b->n_opts; i++) {
    free(b->opts[i].key);
    free(b->opts[i].val);
  }
  free(b->opts);

  loom_arr_free(&b->install_hdrs);
  free(b->prefix);
  free(b->root);
  free(b->cachedir);
  free(b);
}
