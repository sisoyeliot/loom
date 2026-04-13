/*
 * runner.c — Entry point for the compiled build runner.
 *
 * When the user runs `loom build`, the CLI compiles build.c together
 * with libloom.a into a runner binary.  This file provides the main()
 * for that runner.  It initialises the build context, invokes the
 * user-supplied build() function, and then executes the resulting
 * build graph.
 */

#include "internal.h"

extern void build(loom_build_t *b);

int main(int argc, char **argv) {
  loom_build_t *b = loom_setup(argc, argv);
  if (!b)
    return 1;
  build(b);
  int rc = loom_run_build(b);
  loom_teardown(b);
  return rc;
}
