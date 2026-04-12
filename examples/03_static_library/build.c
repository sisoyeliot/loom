#include <loom.h>

void build(loom_build_t *b) {
    loom_target_t *lib = add_static_library(b, "vec");
    add_source(lib, "src/vec2.c");
    add_source(lib, "src/vec3.c");
    add_include_dir(lib, "include");
    set_standard(lib, LOOM_C11);

    loom_target_t *exe = add_executable(b, "demo");
    add_source(exe, "src/main.c");
    add_include_dir(exe, "include");
    set_standard(exe, LOOM_C11);
    link_library(exe, "vec");
    add_ldflag(exe, "-Lbuild");

    set_output_dir(lib, "build");
    set_output_dir(exe, "build");
}
