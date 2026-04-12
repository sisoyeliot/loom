/*
 * loom.h — Loom Build System
 *
 * Public API for defining build targets in your build.c.
 * Include this header and implement void build(loom_build_t *b)
 * to configure how your project should be compiled.
 *
 * Example:
 *   #include <loom.h>
 *
 *   void build(loom_build_t *b) {
 *       loom_target_t *exe = add_executable(b, "myapp");
 *       add_source(exe, "src/main.c");
 *       set_standard(exe, LOOM_C17);
 *       add_cflag(exe, "-Wall");
 *   }
 *
 * All API functions are declared with full loom_ prefix for linker
 * safety.  Short aliases (add_executable, add_source, ...) are
 * provided by default.  Define LOOM_NO_SHORT_NAMES before including
 * this header to disable them.
 */

#ifndef LOOM_H
#define LOOM_H

#define LOOM_VERSION_MAJOR 0
#define LOOM_VERSION_MINOR 1
#define LOOM_VERSION_PATCH 0
#define LOOM_VERSION "0.1.0"

/* ------------------------------------------------------------------ */
/*  Enumerations                                                      */
/* ------------------------------------------------------------------ */

typedef enum {
    LOOM_C89 = 0,
    LOOM_C99,
    LOOM_C11,
    LOOM_C17,
    LOOM_C23
} loom_std_t;

typedef enum {
    LOOM_OPT_NONE = 0,         /* -O0               */
    LOOM_OPT_DEBUG,             /* -Og -g            */
    LOOM_OPT_RELEASE_SAFE,     /* -O2 -g            */
    LOOM_OPT_RELEASE_FAST,     /* -O3               */
    LOOM_OPT_RELEASE_SMALL     /* -Os               */
} loom_opt_t;

typedef enum {
    LOOM_EXECUTABLE = 0,
    LOOM_STATIC_LIB,
    LOOM_SHARED_LIB
} loom_target_type_t;

typedef enum {
    LOOM_OS_UNKNOWN = 0,
    LOOM_OS_LINUX,
    LOOM_OS_MACOS,
    LOOM_OS_FREEBSD,
    LOOM_OS_WINDOWS
} loom_os_t;

typedef enum {
    LOOM_ARCH_UNKNOWN = 0,
    LOOM_ARCH_X86_64,
    LOOM_ARCH_AARCH64,
    LOOM_ARCH_X86,
    LOOM_ARCH_ARM,
    LOOM_ARCH_RISCV64
} loom_arch_t;

typedef enum {
    LOOM_CC_UNKNOWN = 0,
    LOOM_CC_GCC,
    LOOM_CC_CLANG,
    LOOM_CC_TCC
} loom_cc_type_t;

/* ------------------------------------------------------------------ */
/*  Types                                                             */
/* ------------------------------------------------------------------ */

/* Dynamic string array.  Used internally; exposed for transparency. */
typedef struct {
    char **items;
    int    count;
    int    cap;
} loom_arr_t;

/* Key-value pair for user-defined options (-Dkey=value). */
typedef struct {
    char *key;
    char *val;
} loom_kv_t;

/*
 * Detected toolchain information.
 * Populated during runner startup; inspect via get_toolchain().
 */
typedef struct {
    char            cc[512];        /* resolved compiler path       */
    char            ar[512];        /* resolved archiver path       */
    loom_cc_type_t  type;           /* compiler family              */
    int             ver_major;
    int             ver_minor;
    int             ver_patch;
    loom_os_t       os;             /* host operating system        */
    loom_arch_t     arch;           /* host CPU architecture        */
} loom_toolchain_t;

/* Build target (executable or library). */
typedef struct {
    char              *name;
    char              *outname;
    loom_target_type_t type;
    loom_arr_t         sources;
    loom_arr_t         includes;
    loom_arr_t         cflags;
    loom_arr_t         ldflags;
    loom_arr_t         libs;
    loom_arr_t         syslibs;
    loom_std_t         std;
    loom_opt_t         opt;
    char              *outdir;
    int                test;
    int                do_install;
} loom_target_t;

/* Forward-declare for the self-referential function pointer. */
typedef struct loom_step loom_step_t;

/* Custom build step with optional dependency ordering. */
struct loom_step {
    char  *name;
    void (*fn)(loom_step_t *);
    void  *udata;
    int   *deps;
    int    n_deps;
    int    cap_deps;
    int    done;
    int    idx;
};

/* Top-level build context passed to your build() function. */
typedef struct {
    loom_toolchain_t  tc;
    loom_target_t   **targets;
    int               n_targets;
    int               cap_targets;
    loom_step_t     **steps;
    int               n_steps;
    int               cap_steps;
    loom_kv_t        *opts;
    int               n_opts;
    int               cap_opts;
    loom_arr_t        install_hdrs;
    char             *prefix;
    char             *root;
    char             *cachedir;
    int               jobs;
    int               verbose;
    int               release;
    const char       *action;
    int               argc;
    char            **argv;
} loom_build_t;

/* ------------------------------------------------------------------ */
/*  Target creation                                                   */
/* ------------------------------------------------------------------ */

loom_target_t *loom_add_executable(loom_build_t *b, const char *name);
loom_target_t *loom_add_static_library(loom_build_t *b, const char *name);
loom_target_t *loom_add_shared_library(loom_build_t *b, const char *name);

/* ------------------------------------------------------------------ */
/*  Target configuration                                              */
/* ------------------------------------------------------------------ */

/* Add a single source file.  Path is relative to the project root. */
void loom_add_source(loom_target_t *t, const char *path);

/* Add every source file matching a glob pattern (supports **). */
void loom_add_sources(loom_target_t *t, const char *pattern);

/* Append an include search directory (-I). */
void loom_add_include_dir(loom_target_t *t, const char *dir);

/* Append a raw compiler flag. */
void loom_add_cflag(loom_target_t *t, const char *flag);

/* Append a raw linker flag. */
void loom_add_ldflag(loom_target_t *t, const char *flag);

/* Link against a library by name (-l). */
void loom_link_library(loom_target_t *t, const char *lib);

/* Link a system library; tries pkg-config, falls back to -l. */
void loom_link_system_library(loom_target_t *t, const char *name);

/* Set the C language standard for this target. */
void loom_set_standard(loom_target_t *t, loom_std_t std);

/* Set the optimisation level for this target. */
void loom_set_optimization(loom_target_t *t, loom_opt_t opt);

/* Override the output directory for this target's artifact. */
void loom_set_output_dir(loom_target_t *t, const char *dir);

/* Override the output file name (e.g. produce "loom" from target "loom-cli"). */
void loom_set_output_name(loom_target_t *t, const char *name);

/* Mark this target as a test (only built/run via `loom test`). */
void loom_set_test(loom_target_t *t);

/* Mark this target for installation with `loom install`. */
void loom_install_artifact(loom_target_t *t);

/* ------------------------------------------------------------------ */
/*  Build context                                                     */
/* ------------------------------------------------------------------ */

/* Return the detected toolchain descriptor. */
const loom_toolchain_t *loom_get_toolchain(loom_build_t *b);

loom_os_t   loom_get_os(loom_build_t *b);
loom_arch_t loom_get_arch(loom_build_t *b);

/* Set the installation prefix (default /usr/local). */
void loom_set_install_prefix(loom_build_t *b, const char *prefix);

/* Read a boolean -Dname=true|false option.  Returns def if absent. */
int loom_option_bool(loom_build_t *b, const char *name, int def);

/* Read a string -Dname=value option.  Returns def if absent. */
const char *loom_option_str(loom_build_t *b, const char *name, const char *def);

/* Queue a header file for installation into PREFIX/include. */
void loom_install_header(loom_build_t *b, const char *path);

/* ------------------------------------------------------------------ */
/*  Custom build steps                                                */
/* ------------------------------------------------------------------ */

loom_step_t *loom_add_step(loom_build_t *b, const char *name,
                           void (*fn)(loom_step_t *));
void  loom_step_depends_on(loom_step_t *step, loom_step_t *dep);
void  loom_step_set_userdata(loom_step_t *step, void *data);
void *loom_step_get_userdata(loom_step_t *step);

/* ------------------------------------------------------------------ */
/*  Short aliases (disable with #define LOOM_NO_SHORT_NAMES)          */
/* ------------------------------------------------------------------ */

#ifndef LOOM_NO_SHORT_NAMES
#define add_executable      loom_add_executable
#define add_static_library  loom_add_static_library
#define add_shared_library  loom_add_shared_library
#define add_source          loom_add_source
#define add_sources         loom_add_sources
#define add_include_dir     loom_add_include_dir
#define add_cflag           loom_add_cflag
#define add_ldflag          loom_add_ldflag
#define link_library        loom_link_library
#define link_system_library loom_link_system_library
#define set_standard        loom_set_standard
#define set_optimization    loom_set_optimization
#define set_output_dir      loom_set_output_dir
#define set_output_name     loom_set_output_name
#define set_test            loom_set_test
#define install_artifact    loom_install_artifact
#define get_toolchain       loom_get_toolchain
#define get_os              loom_get_os
#define get_arch            loom_get_arch
#define set_install_prefix  loom_set_install_prefix
#define option_bool         loom_option_bool
#define option_str          loom_option_str
#define install_header      loom_install_header
#define add_step            loom_add_step
#define step_depends_on     loom_step_depends_on
#define step_set_userdata   loom_step_set_userdata
#define step_get_userdata   loom_step_get_userdata
#endif

#endif /* LOOM_H */
