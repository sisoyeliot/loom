#include <loom.h>

void build(loom_build_t *b) {
    loom_target_t *exe = add_executable(b, "hello");
    add_source(exe, "main.c");
    set_standard(exe, LOOM_C11);
}
