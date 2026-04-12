#include <loom.h>
#include <stdio.h>
#include <string.h>

void build(loom_build_t *b) {
  int use_colors = option_bool(b, "colors", 1);
  const char *greeting = option_str(b, "greeting", "World");

  loom_target_t *exe = add_executable(b, "greeter");
  add_source(exe, "main.c");
  set_standard(exe, LOOM_C11);

  if (use_colors)
    add_cflag(exe, "-DUSE_COLORS");

  char define[256];
  snprintf(define, sizeof(define), "-DGREETING=\"%s\"", greeting);
  add_cflag(exe, define);
}
