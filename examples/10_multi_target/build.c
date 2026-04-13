#include <loom.h>

void build(loom_build_t *b) {
  loom_target_t *lib = add_static_library(b, "strutil");
  add_source(lib, "src/strutil.c");
  add_include_dir(lib, "include");
  set_standard(lib, LOOM_C11);
  set_output_dir(lib, "build");

  loom_target_t *cli = add_executable(b, "strcli");
  add_source(cli, "src/main.c");
  add_include_dir(cli, "include");
  set_standard(cli, LOOM_C11);
  link_library(cli, "strutil");
  add_ldflag(cli, "-Lbuild");
  set_output_dir(cli, "build");

  loom_target_t *test = add_executable(b, "tests");
  add_source(test, "tests/test_strutil.c");
  add_include_dir(test, "include");
  set_standard(test, LOOM_C11);
  link_library(test, "strutil");
  add_ldflag(test, "-Lbuild");
  set_output_dir(test, "build");
  set_test(test);
}
