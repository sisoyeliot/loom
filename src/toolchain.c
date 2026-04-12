#include "internal.h"
#include <unistd.h>

static void trim_newline(char *s) {
    char *nl = strchr(s, '\n');
    if (nl) *nl = '\0';
}

static int resolve_path(const char *name, char *out, size_t outsz) {
    char *argv[] = {"which", (char *)name, NULL};
    char buf[512];
    if (loom_exec_capture(argv, buf, sizeof(buf)) == 0 && buf[0]) {
        trim_newline(buf);
        snprintf(out, outsz, "%s", buf);
        return 0;
    }
    return -1;
}

static int try_compiler(const char *cc, loom_toolchain_t *tc) {
    char *argv[] = {(char *)cc, "--version", NULL};
    char buf[2048];
    if (loom_exec_capture(argv, buf, sizeof(buf)) != 0)
        return -1;

    char resolved[512];
    if (resolve_path(cc, resolved, sizeof(resolved)) == 0)
        snprintf(tc->cc, sizeof(tc->cc), "%s", resolved);
    else
        snprintf(tc->cc, sizeof(tc->cc), "%s", cc);

    tc->type = LOOM_CC_UNKNOWN;
    tc->ver_major = tc->ver_minor = tc->ver_patch = 0;

    if (strstr(buf, "clang") || strstr(buf, "Clang")) {
        tc->type = LOOM_CC_CLANG;
        char *v = strstr(buf, "version ");
        if (v) sscanf(v + 8, "%d.%d.%d", &tc->ver_major, &tc->ver_minor, &tc->ver_patch);
    } else if (strstr(buf, "gcc") || strstr(buf, "GCC")) {
        tc->type = LOOM_CC_GCC;
        char *v = strstr(buf, ") ");
        if (v) sscanf(v + 2, "%d.%d.%d", &tc->ver_major, &tc->ver_minor, &tc->ver_patch);
        if (tc->ver_major == 0) {
            v = strstr(buf, "version ");
            if (v) sscanf(v + 8, "%d.%d.%d", &tc->ver_major, &tc->ver_minor, &tc->ver_patch);
        }
    } else if (strstr(buf, "tcc")) {
        tc->type = LOOM_CC_TCC;
        char *v = strstr(buf, "version ");
        if (v) sscanf(v + 8, "%d.%d.%d", &tc->ver_major, &tc->ver_minor, &tc->ver_patch);
    }

    return 0;
}

static void detect_ar(loom_toolchain_t *tc) {
    const char *ar = getenv("AR");
    if (ar && ar[0]) {
        snprintf(tc->ar, sizeof(tc->ar), "%s", ar);
        return;
    }

    char resolved[512];
    if (resolve_path("ar", resolved, sizeof(resolved)) == 0)
        snprintf(tc->ar, sizeof(tc->ar), "%s", resolved);
    else
        snprintf(tc->ar, sizeof(tc->ar), "ar");
}

int loom_detect_toolchain(loom_toolchain_t *tc) {
    memset(tc, 0, sizeof(*tc));

    tc->os = LOOM_OS_UNKNOWN;
#if defined(__APPLE__)
    tc->os = LOOM_OS_MACOS;
#elif defined(__linux__)
    tc->os = LOOM_OS_LINUX;
#elif defined(__FreeBSD__)
    tc->os = LOOM_OS_FREEBSD;
#elif defined(_WIN32)
    tc->os = LOOM_OS_WINDOWS;
#endif

    tc->arch = LOOM_ARCH_UNKNOWN;
#if defined(__x86_64__) || defined(_M_X64)
    tc->arch = LOOM_ARCH_X86_64;
#elif defined(__aarch64__) || defined(_M_ARM64)
    tc->arch = LOOM_ARCH_AARCH64;
#elif defined(__i386__) || defined(_M_IX86)
    tc->arch = LOOM_ARCH_X86;
#elif defined(__arm__)
    tc->arch = LOOM_ARCH_ARM;
#elif defined(__riscv) && __riscv_xlen == 64
    tc->arch = LOOM_ARCH_RISCV64;
#endif

    const char *cc;

    cc = getenv("LOOM_CC");
    if (cc && cc[0] && try_compiler(cc, tc) == 0) goto found;

    cc = getenv("CC");
    if (cc && cc[0] && try_compiler(cc, tc) == 0) goto found;

    {
        static const char *candidates[] = {"cc", "gcc", "clang", "tcc", NULL};
        for (int i = 0; candidates[i]; i++)
            if (try_compiler(candidates[i], tc) == 0) goto found;
    }

    return -1;

found:
    detect_ar(tc);
    return 0;
}
