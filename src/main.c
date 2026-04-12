#include "internal.h"
#include <unistd.h>
#include <sys/stat.h>

#ifndef LOOM_PREFIX
#define LOOM_PREFIX "/usr/local"
#endif

#define EQ(a, b) (strcmp((a), (b)) == 0)

static void version(void) {
    printf("loom %s\n", LOOM_VERSION);
}

static void usage(void) {
    printf("loom %s — build system for C\n\n", LOOM_VERSION);
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
    if (cc && cc[0]) return cc;
    cc = getenv("CC");
    if (cc && cc[0]) return cc;

    static const char *try[] = {"cc", "gcc", "clang", "tcc", NULL};
    for (int i = 0; try[i]; i++) {
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "command -v %s >/dev/null 2>&1", try[i]);
        if (system(cmd) == 0) return try[i];
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
    if (system("command -v clang-format >/dev/null 2>&1") != 0) {
        LOOM_ERR("clang-format not found in PATH");
        return 1;
    }
    return system(
        "find . -name '*.c' -o -name '*.h' | grep -v .loom | xargs clang-format -i"
    );
}

static int cmd_dispatch(const char *cmd, int argc, char **argv) {
    if (!loom_path_exists("build.c")) {
        LOOM_ERR("no build.c in current directory (run 'loom init' first)");
        return 1;
    }

    loom_mkdir_p(".loom");

    int need_rebuild = !loom_path_exists(".loom/runner");
    if (!need_rebuild) {
        struct stat sc, sr;
        if (stat("build.c", &sc) == 0 && stat(".loom/runner", &sr) == 0)
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
                 " -I" LOOM_PREFIX "/include"
                 " -L" LOOM_PREFIX "/lib"
                 " -lloom -o .loom/runner",
                 cc);
        int rc = system(compile);
        if (rc != 0) {
            LOOM_ERR("failed to compile build.c");
            return 1;
        }
    }

    int n = argc + 3;
    char **rav = calloc(n, sizeof(char *));
    rav[0] = ".loom/runner";
    rav[1] = (char *)cmd;
    for (int i = 0; i < argc; i++) rav[i + 2] = argv[i];
    rav[n - 1] = NULL;

    execv(".loom/runner", rav);
    LOOM_ERR("failed to execute runner");
    free(rav);
    return 1;
}

int main(int argc, char **argv) {
    if (argc < 2) { usage(); return 1; }

    const char *cmd = argv[1];

    if (EQ(cmd, "help") || EQ(cmd, "-h") || EQ(cmd, "--help")) {
        usage(); return 0;
    }
    if (EQ(cmd, "version") || EQ(cmd, "-V") || EQ(cmd, "--version")) {
        version(); return 0;
    }
    if (EQ(cmd, "init")) {
        return loom_cmd_init(argc > 2 ? argv[2] : ".");
    }
    if (EQ(cmd, "create")) {
        if (argc < 3) { LOOM_ERR("usage: loom create <name>"); return 1; }
        return loom_cmd_create(argv[2]);
    }
    if (EQ(cmd, "clean")) return cmd_clean();
    if (EQ(cmd, "fmt"))   return cmd_fmt();

    if (EQ(cmd, "build") || EQ(cmd, "run") || EQ(cmd, "test") ||
        EQ(cmd, "install") || EQ(cmd, "query")) {
        return cmd_dispatch(cmd, argc - 2, argv + 2);
    }

    LOOM_ERR("unknown command '%s' (try 'loom help')", cmd);
    return 1;
}
