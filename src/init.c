#include "internal.h"
#include <sys/stat.h>

static void write_file(const char *path, const char *content) {
    char *dir = loom_path_dir(path);
    loom_mkdir_p(dir);
    free(dir);
    FILE *f = fopen(path, "w");
    if (!f) { LOOM_ERR("cannot create %s", path); return; }
    fputs(content, f);
    fclose(f);
}

int loom_cmd_init(const char *dir) {
    char *build_c   = loom_path_join(dir, "build.c");
    char *main_c    = loom_path_join(dir, "src/main.c");
    char *gitignore = loom_path_join(dir, ".gitignore");

    if (loom_path_exists(build_c)) {
        LOOM_ERR("build.c already exists in %s", dir);
        free(build_c); free(main_c); free(gitignore);
        return 1;
    }

    write_file(build_c,
        "#include <loom.h>\n"
        "\n"
        "void build(loom_build_t *b) {\n"
        "    loom_target_t *exe = add_executable(b, \"app\");\n"
        "\n"
        "    add_source(exe, \"src/main.c\");\n"
        "    set_standard(exe, LOOM_C11);\n"
        "    set_optimization(exe, LOOM_OPT_DEBUG);\n"
        "    add_cflag(exe, \"-Wall\");\n"
        "    add_cflag(exe, \"-Wextra\");\n"
        "}\n"
    );

    write_file(main_c,
        "#include <stdio.h>\n"
        "\n"
        "int main(void) {\n"
        "    printf(\"Hello from loom!\\n\");\n"
        "    return 0;\n"
        "}\n"
    );

    if (!loom_path_exists(gitignore)) {
        write_file(gitignore,
            ".loom/\n"
            "build/\n"
            "*.o\n"
            "*.a\n"
            "*.so\n"
            "*.dylib\n"
        );
    }

    LOOM_OK("initialized project in %s", dir);
    free(build_c); free(main_c); free(gitignore);
    return 0;
}

int loom_cmd_create(const char *name) {
    if (loom_path_exists(name)) {
        LOOM_ERR("directory '%s' already exists", name);
        return 1;
    }
    loom_mkdir_p(name);
    return loom_cmd_init(name);
}
