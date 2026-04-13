#include <loom.h>
#include <stdio.h>

void build(loom_build_t *b) {
  const char *prefix = option_str(b, "prefix", "/usr/local");
  set_install_prefix(b, prefix);

  char pflag[4096];
  snprintf(pflag, sizeof(pflag), "-DLOOM_PREFIX=\"%s\"", prefix);

  loom_target_t *lib = add_static_library(b, "loom");
  add_source(lib, "src/loom.c");
  add_source(lib, "src/runner.c");
  add_source(lib, "src/toolchain.c");
  add_source(lib, "src/hash.c");
  add_source(lib, "src/cache.c");
  add_source(lib, "src/exec.c");
  add_source(lib, "src/path.c");
  add_include_dir(lib, "include");
  set_standard(lib, LOOM_C11);
  set_optimization(lib, LOOM_OPT_RELEASE_FAST);
  add_cflag(lib, "-Wall");
  add_cflag(lib, "-Wextra");
  set_output_dir(lib, "build");
  install_artifact(lib);

  loom_target_t *cli = add_executable(b, "loom-cli");
  add_source(cli, "src/main.c");
  add_source(cli, "src/init.c");
  add_source(cli, "src/path.c");
  add_source(cli, "src/exec.c");
  add_include_dir(cli, "include");
  set_standard(cli, LOOM_C11);
  set_optimization(cli, LOOM_OPT_RELEASE_FAST);
  add_cflag(cli, "-Wall");
  add_cflag(cli, "-Wextra");
  add_cflag(cli, pflag);
  set_output_dir(cli, "build");
  set_output_name(cli, "loom");
  install_artifact(cli);

  install_header(b, "include/loom.h");
}
