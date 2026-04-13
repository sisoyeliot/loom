#include <loom.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

static void ensure_dir(const char *dir) { mkdir(dir, 0755); }

void build(loom_build_t *b) {
  ensure_dir("generated");

  FILE *f = fopen("generated/config.h", "w");
  if (f) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    const loom_toolchain_t *tc = get_toolchain(b);

    fprintf(f, "#ifndef CONFIG_H\n");
    fprintf(f, "#define CONFIG_H\n\n");
    fprintf(f, "#define BUILD_DATE \"%04d-%02d-%02d\"\n", t->tm_year + 1900,
            t->tm_mon + 1, t->tm_mday);
    fprintf(f, "#define BUILD_TIME \"%02d:%02d:%02d\"\n", t->tm_hour, t->tm_min,
            t->tm_sec);
    fprintf(f, "#define BUILD_COMPILER \"%s\"\n", tc->cc);

    const char *os = "unknown";
    if (tc->os == LOOM_OS_LINUX)
      os = "linux";
    if (tc->os == LOOM_OS_MACOS)
      os = "macos";
    if (tc->os == LOOM_OS_FREEBSD)
      os = "freebsd";
    if (tc->os == LOOM_OS_WINDOWS)
      os = "windows";

    const char *arch = "unknown";
    if (tc->arch == LOOM_ARCH_X86_64)
      arch = "x86_64";
    if (tc->arch == LOOM_ARCH_AARCH64)
      arch = "aarch64";
    if (tc->arch == LOOM_ARCH_X86)
      arch = "x86";
    if (tc->arch == LOOM_ARCH_ARM)
      arch = "arm";

    fprintf(f, "#define BUILD_OS \"%s\"\n", os);
    fprintf(f, "#define BUILD_ARCH \"%s\"\n", arch);
    fprintf(f, "\n#endif\n");
    fclose(f);
  }

  loom_target_t *exe = add_executable(b, "app");
  add_source(exe, "main.c");
  add_include_dir(exe, "generated");
  set_standard(exe, LOOM_C11);
}
