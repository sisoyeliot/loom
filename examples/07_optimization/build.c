#include <loom.h>
#include <string.h>

void build(loom_build_t *b) {
    const char *mode = option_str(b, "mode", "debug");
    loom_opt_t opt;

    if (strcmp(mode, "debug") == 0)        opt = LOOM_OPT_DEBUG;
    else if (strcmp(mode, "safe") == 0)    opt = LOOM_OPT_RELEASE_SAFE;
    else if (strcmp(mode, "fast") == 0)    opt = LOOM_OPT_RELEASE_FAST;
    else if (strcmp(mode, "small") == 0)   opt = LOOM_OPT_RELEASE_SMALL;
    else                                   opt = LOOM_OPT_NONE;

    loom_target_t *exe = add_executable(b, "bench");
    add_source(exe, "main.c");
    set_standard(exe, LOOM_C11);
    set_optimization(exe, opt);
    add_cflag(exe, "-Wall");
}
