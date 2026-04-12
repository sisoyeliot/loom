#include <loom.h>

void build(loom_build_t *b) {
  loom_target_t *lib = add_shared_library(b, "greet");
  add_source(lib, "src/greet.c");
  add_include_dir(lib, "include");
  set_standard(lib, LOOM_C11);
  set_output_dir(lib, "build");

  loom_target_t *exe = add_executable(b, "app");
  add_source(exe, "src/main.c");
  add_include_dir(exe, "include");
  set_standard(exe, LOOM_C11);
  link_library(exe, "greet");
  add_ldflag(exe, "-Lbuild");
  add_ldflag(exe, "-Wl,-rpath,build");
  set_output_dir(exe, "build");
}
