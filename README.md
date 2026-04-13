# Loom

Loom is a fast, zero-dependency C/C++ build system designed to be simple, self-hosting, and profoundly portable. Instead of relying on a bespoke scripting language or external dependencies like Python or CMake, Loom configures its builds completely via a standard C file (`build.c`). 

If you know C, you already know how to write a build script in Loom.

## Features

- **No Dependencies:** A standard C compiler is all you need to start building.
- **Configured in C:** Write your build configurations using the fully featured C language.
- **Short & Intuitive API:** Easy to use functions to define executables, static libraries, and shared libraries. Short aliases available by default (e.g., `add_executable`, `add_source`).
- **Compiler Auto-Detection:** Automatically discovers the compiler toolchain, system architecture, and host operating system.
- **Robust Feature Set:** Supports globbing source files, parsing build options natively (`-Dkey=val`), linking system libraries, defining custom build steps, setting C standards, and handling optimization levels natively.
- **Self-Hosting:** Loom builds itself!

## Getting Started

A basic project setup looks like this:

1. Create a `build.c` file at the root of your project:

```c
#include <loom.h>

void build(loom_build_t *b) {
    // Define an executable target named "myapp"
    loom_target_t *exe = add_executable(b, "myapp");
    
    // Add C source files
    add_source(exe, "src/main.c");
    // You can also glob multiple sources: add_sources(exe, "src/**/*.c");
    
    // Configure standard and optimization
    set_standard(exe, LOOM_C17);
    set_optimization(exe, LOOM_OPT_RELEASE_FAST);
    
    // Add compiler warning flags
    add_cflag(exe, "-Wall");
    add_cflag(exe, "-Wextra");
    
    // Define where to output the build artifacts
    set_output_dir(exe, "build");
}
```

2. Compile and run your build using Loom!

## Core API Overview

All API functions are prefixed internally (`loom_...`) for linker safety, with short aliases exposed by default for convenience. (If you prefer to strictly use prefixed names, define `LOOM_NO_SHORT_NAMES` before including `loom.h`).

### Targets
- `add_executable(b, name)` 
- `add_static_library(b, name)`
- `add_shared_library(b, name)`

### Configuration
- `add_source(t, path)`
- `add_sources(t, pattern)`
- `add_include_dir(t, dir)`
- `add_cflag(t, flag)` / `add_ldflag(t, flag)`
- `set_standard(t, std)`
- `set_optimization(t, opt)`

### Linking
- `link_library(t, lib)` - link an internal library
- `link_system_library(t, name)` - link an external/system library using automatic pkg-config fallbacks. 

### Custom Build Steps
Loom lets you define discrete custom build actions and specify their execution order:
- `add_step(b, name, fn)`
- `step_depends_on(step, dep)`

## Examples

The `examples/` directory contains various self-contained configurations demonstrating:
- Basic binaries (`01_hello`)
- Working with Static/Shared Libraries (`03_static_library`, `04_shared_library`)
- Globbing sources natively (`05_glob_sources`)
- Using build-time Options (`06_options`)
- Linking System Libraries (`08_system_library`)
- Defining custom build steps (`09_custom_steps`)

## License

Please refer to the `LICENSE` file for details.