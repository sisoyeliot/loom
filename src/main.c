#include "internal.h"

#ifndef _WIN32
#include <unistd.h>
#endif
#include <sys/stat.h>

#ifdef _WIN32
#include <process.h>
#endif

#ifndef LOOM_PREFIX
#define LOOM_PREFIX /usr/local
#endif

#define STR(s) #s
#define XSTR(s) STR(s)

#define EQ(a, b) (strcmp((a), (b)) == 0)

static void version(void) { printf("loom %s\n", LOOM_VERSION); }

static void usage(void) {
  printf("loom %s - build system for C\n\n", LOOM_VERSION);
  printf("usage: loom <command> [options]\n\n");
  printf("commands:\n");
  printf("  build          Compile the project\n");
  printf("  run [-- args]  Build and run the first executable\n");
  printf("  test           Build and run test targets\n");
  printf("  init [dir]     Initialize a project in the given directory\n");
  printf("  create <name>  Create a new project directory\n");
  printf("  clean          Remove build artifacts\n");
  printf("  install        Install built artifacts\n");
  printf("  query          Show toolchain and target info\n");
  printf("  fmt            Format sources with clang-format\n");
  printf("  help           Show this message\n");
  printf("  version        Show version\n");
  printf("\noptions:\n");
  printf("  -v, --verbose  Show compiler commands\n");
  printf("  -j N           Set max parallel jobs\n");
  printf("  --release      Build in release mode\n");
  printf("  -Dkey=value    Pass option to build.c\n");
}

static const char *find_cc(void) {
  const char *cc = getenv("LOOM_CC");
  if (cc && cc[0])
    return cc;
  cc = getenv("CC");
  if (cc && cc[0])
    return cc;

  static const char *try[] = {"cc", "gcc", "clang", "tcc", NULL};
  for (int i = 0; try[i]; i++) {
    char cmd[256];
#ifdef _WIN32
    snprintf(cmd, sizeof(cmd), "where %s >nul 2>&1", try[i]);
#else
    snprintf(cmd, sizeof(cmd), "command -v %s >/dev/null 2>&1", try[i]);
#endif
    if (system(cmd) == 0)
      return try[i];
  }
  return NULL;
}

static int cmd_clean(void) {
  if (loom_path_exists(".loom")) {
    loom_rmrf(".loom");
    LOOM_OK("cleaned .loom/");
  }
  return 0;
}

static int cmd_fmt(void) {
#ifdef _WIN32
  if (system("where clang-format >nul 2>&1") != 0) {
#else
  if (system("command -v clang-format >/dev/null 2>&1") != 0) {
#endif
    LOOM_ERR("clang-format not found in PATH");
    return 1;
  }
  loom_arr_t arr = {0};
  loom_glob_match("**/*.c", &arr);
  loom_glob_match("**/*.h", &arr);

  loom_arr_t filtered = {0};
  for (int i = 0; i < arr.count; i++) {
    if (!strstr(arr.items[i], ".loom")) {
      loom_arr_push(&filtered, arr.items[i]);
    }
  }

  if (filtered.count == 0) {
    loom_arr_free(&arr);
    return 0;
  }

  char **fmt_argv = calloc(filtered.count + 3, sizeof(char *));
  fmt_argv[0] = strdup("clang-format");
  fmt_argv[1] = strdup("-i");
  for (int i = 0; i < filtered.count; i++) {
    fmt_argv[i + 2] = strdup(filtered.items[i]);
  }
  fmt_argv[filtered.count + 2] = NULL;

  int rc = loom_exec(fmt_argv);

  for (int i = 0; fmt_argv[i]; i++)
    free(fmt_argv[i]);
  free(fmt_argv);
  loom_arr_free(&arr);
  loom_arr_free(&filtered);
  return rc;
}

static int cmd_dispatch(const char *cmd, int argc, char **argv) {
  if (!loom_path_exists("build.c")) {
    LOOM_ERR("no build.c in current directory (run 'loom init' first)");
    return 1;
  }

  loom_mkdir_p(".loom");

#ifdef _WIN32
#define RUNNER_OUT ".loom/runner.exe"
#else
#define RUNNER_OUT ".loom/runner"
#endif

  int need_rebuild = !loom_path_exists(RUNNER_OUT);
  if (!need_rebuild) {
    struct stat sc, sr;
    if (stat("build.c", &sc) == 0 && stat(RUNNER_OUT, &sr) == 0)
      need_rebuild = sc.st_mtime > sr.st_mtime;
  }

  if (need_rebuild) {
    const char *cc = find_cc();
    if (!cc) {
      LOOM_ERR("no C compiler found (set CC or LOOM_CC)");
      return 1;
    }

    LOOM_INFO("compiling build runner...");
    char compile[4096];
    snprintf(compile, sizeof(compile),
             "%s -std=c11 -O2 build.c"
             " -I" XSTR(LOOM_PREFIX) "/include"
             " -L" XSTR(LOOM_PREFIX) "/lib"
             " -lloom -o %s",
             cc, RUNNER_OUT);
    int rc = system(compile);
    if (rc != 0) {
      LOOM_ERR("failed to compile build.c");
      return 1;
    }
  }

  int n = argc + 3;
  char **rav = calloc(n, sizeof(char *));
  rav[0] = RUNNER_OUT;
  rav[1] = (char *)cmd;
  for (int i = 0; i < argc; i++)
    rav[i + 2] = argv[i];
  rav[n - 1] = NULL;

#ifdef _WIN32
  int rc = loom_exec((char **)rav);
  free(rav);
  return rc;
#else
  execv(RUNNER_OUT, rav);
  LOOM_ERR("failed to execute runner");
  free(rav);
  return 1;
#endif
}

int main(int argc, char **argv) {
  if (argc < 2) {
    usage();
    return 1;
  }

  const char *cmd = argv[1];

  if (EQ(cmd, "help") || EQ(cmd, "-h") || EQ(cmd, "--help")) {
    usage();
    return 0;
  }
  if (EQ(cmd, "version") || EQ(cmd, "-V") || EQ(cmd, "--version")) {
    version();
    return 0;
  }
  if (EQ(cmd, "init")) {
    return loom_cmd_init(argc > 2 ? argv[2] : ".");
  }
  if (EQ(cmd, "create")) {
    if (argc < 3) {
      LOOM_ERR("usage: loom create <name>");
      return 1;
    }
    return loom_cmd_create(argv[2]);
  }
  if (EQ(cmd, "clean"))
    return cmd_clean();
  if (EQ(cmd, "fmt"))
    return cmd_fmt();

  if (EQ(cmd, "build") || EQ(cmd, "run") || EQ(cmd, "test") ||
      EQ(cmd, "install") || EQ(cmd, "query")) {
    return cmd_dispatch(cmd, argc - 2, argv + 2);
  }

  LOOM_ERR("unknown command '%s' (try 'loom help')", cmd);
  return 1;
}
