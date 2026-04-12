#include <loom.h>

void build(loom_build_t *b) {
  loom_target_t *exe = add_executable(b, "calc");

  add_source(exe, "src/main.c");
  add_source(exe, "src/math_ops.c");
  add_source(exe, "src/print.c");

  add_include_dir(exe, "include");
  set_standard(exe, LOOM_C11);
  add_cflag(exe, "-Wall");
  add_cflag(exe, "-Wextra");
}
