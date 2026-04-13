#include <loom.h>

void build(loom_build_t *b) {
  loom_target_t *exe = loom_add_executable(b, "shader_demo");

  loom_add_source(exe, "src/main.c");

  loom_add_include_dir(exe, "vendor/raylib/include");
  loom_add_ldflag(exe, "-Lvendor/raylib/lib");

  loom_link_library(exe, "raylib");
#ifdef _WIN32
  loom_link_library(exe, "winmm");
  loom_link_library(exe, "gdi32");
  loom_link_library(exe, "opengl32");
#elif defined(__APPLE__)
  loom_add_ldflag(exe, "-framework");
  loom_add_ldflag(exe, "CoreVideo");
  loom_add_ldflag(exe, "-framework");
  loom_add_ldflag(exe, "IOKit");
  loom_add_ldflag(exe, "-framework");
  loom_add_ldflag(exe, "Cocoa");
  loom_add_ldflag(exe, "-framework");
  loom_add_ldflag(exe, "GLUT");
  loom_add_ldflag(exe, "-framework");
  loom_add_ldflag(exe, "OpenGL");
#else
  loom_link_library(exe, "GL");
  loom_link_library(exe, "m");
  loom_link_library(exe, "pthread");
  loom_link_library(exe, "dl");
  loom_link_library(exe, "rt");
  loom_link_library(exe, "X11");
#endif

  loom_set_standard(exe, LOOM_C11);
  loom_set_optimization(exe, LOOM_OPT_RELEASE_FAST);

  loom_set_output_dir(exe, "build");
}
