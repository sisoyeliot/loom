#include <loom.h>

void build(loom_build_t *b) {
    loom_target_t *exe = add_executable(b, "app");

    add_sources(exe, "src/**/*.c");
    add_include_dir(exe, "include");
    set_standard(exe, LOOM_C11);
    add_cflag(exe, "-Wall");
}
