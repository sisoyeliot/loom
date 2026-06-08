# Loom

![C](https://img.shields.io/badge/c-00599C?style=for-the-badge&logo=c&logoColor=white)
![Version](https://img.shields.io/badge/v0.1.0-early_stage-orange?style=for-the-badge)
![Platform](https://img.shields.io/badge/linux_%7C_macos_%7C_windows-grey?style=for-the-badge)
![License](https://img.shields.io/badge/license-MIT-green?style=for-the-badge)

A fast, zero-dependency C/C++ build system that uses plain C as its configuration language. Instead of learning a bespoke DSL or relying on external tools like CMake or Python, Loom lets you define your entire build pipeline in a standard `build.c` file.

**If you know C, you already know how to use Loom.**

> [!WARNING]
> This project is in an early stage of development. It is functional and can build real projects, but it has not yet undergone extensive testing or security auditing. See the [Security Policy](SECURITY.md) for details.

## Features

- **Zero Dependencies** - A standard C compiler is the only prerequisite. No Python, no Java, no Ruby.

- **Configured in C** - Write build scripts using the fully featured C language: variables, loops, conditionals, string formatting, and the entire standard library are at your disposal.

- **Intuitive API** - Clean, discoverable functions to define executables, static libraries, and shared libraries. Short aliases are provided by default (e.g., `add_executable`, `add_source`).

- **Compiler Auto-Detection** - Automatically discovers the compiler toolchain (GCC, Clang, TCC), system architecture (x86_64, AArch64, RISC-V, ARM), and host operating system.

- **Robust Feature Set** - Native support for globbing source files, parsing build-time options (`-Dkey=val`), linking system libraries with pkg-config fallback, defining custom build steps with dependency ordering, setting C standards (C89–C23), and managing optimization levels.

- **Self-Hosting** - Loom builds itself. The project's own [`build.c`](build.c) serves as a real-world usage example.

- **Cross-Platform** - Works on Linux, macOS, FreeBSD, and Windows with first-class support for GCC, Clang, TCC, and MSVC.

## Quick Start

### 1. Bootstrap & Install

Loom needs to be bootstrapped once using a standard C compiler. After that, it can rebuild itself.

<details>
<summary><strong>Linux / macOS / FreeBSD</strong></summary>

```bash
git clone https://github.com/sisoyeliot/loom.git
cd loom

# Bootstrap with the bundled shell script
./bootstrap.sh

# Install system-wide (default: /usr/local)
sudo make install

# After installation, loom can rebuild itself:
loom build
loom install -Dprefix=$HOME/.local
```
</details>

<details>
<summary><strong>Windows (PowerShell)</strong></summary>

```powershell
git clone https://github.com/sisoyeliot/loom.git
cd loom

# Bootstrap (auto-detects gcc, clang, or cl)
.\bootstrap.ps1

# Install (default: %LOCALAPPDATA%\loom, auto-adds to PATH)
.\bootstrap.ps1 install

# After installation:
loom build
loom install -Dprefix=C:\mytools
```
</details>

### 2. Initialize a New Project

```bash
# Initialize in the current directory
loom init

# Or create a new project from scratch
loom create myapp
```

### 3. Write Your Build Configuration

Create a `build.c` file at the root of your project:

```c
#include <loom.h>

void build(loom_build_t *b) {
    // Define an executable target
    loom_target_t *exe = add_executable(b, "myapp");

    // Add source files
    add_source(exe, "src/main.c");
    // Or glob an entire directory: add_sources(exe, "src/**/*.c");

    // Configure the build
    set_standard(exe, LOOM_C17);
    set_optimization(exe, LOOM_OPT_RELEASE_FAST);

    // Compiler flags
    add_cflag(exe, "-Wall");
    add_cflag(exe, "-Wextra");

    // Output directory
    set_output_dir(exe, "build");
}
```

### 4. Build & Run

```bash
loom build            # Compile the project
loom run              # Build and run the first executable
loom run -- arg1 arg2 # Pass arguments to your program
```

## CLI Reference

```
loom <command> [options]

Commands:
  build          Compile the project
  run [-- args]  Build and run the first executable target
  test           Build and run test targets
  init [dir]     Initialize a project in the given directory
  create <name>  Create a new project directory
  clean          Remove build artifacts (.loom/)
  install        Install built artifacts to PREFIX
  query          Show detected toolchain and target info
  fmt            Format all .c/.h sources with clang-format
  help           Show help message
  version        Show version

Options:
  -v, --verbose  Show compiler commands
  -j N           Set max parallel jobs
  --release      Build in release mode
  -Dkey=value    Pass a build-time option to build.c
```

## API Reference

All API functions use a `loom_` prefix internally for linker safety. Short aliases (without the prefix) are provided by default. Define `LOOM_NO_SHORT_NAMES` before including `loom.h` to disable them.

### Target Creation

| Function | Description |
|---|---|
| `add_executable(b, name)` | Define a new executable target |
| `add_static_library(b, name)` | Define a new static library (`.a` / `.lib`) |
| `add_shared_library(b, name)` | Define a new shared library (`.so` / `.dylib` / `.dll`) |

### Target Configuration

| Function | Description |
|---|---|
| `add_source(t, path)` | Add a single source file |
| `add_sources(t, pattern)` | Add sources matching a glob pattern (supports `**`) |
| `add_include_dir(t, dir)` | Append an include search directory (`-I`) |
| `add_cflag(t, flag)` | Append a raw compiler flag |
| `add_ldflag(t, flag)` | Append a raw linker flag |
| `set_standard(t, std)` | Set the C standard (`LOOM_C89` … `LOOM_C23`) |
| `set_optimization(t, opt)` | Set the optimization level |
| `set_output_dir(t, dir)` | Override the output directory for build artifacts |
| `set_output_name(t, name)` | Override the output file name |
| `set_test(t)` | Mark this target as a test (built only via `loom test`) |
| `install_artifact(t)` | Mark this target for installation via `loom install` |

### Linking

| Function | Description |
|---|---|
| `link_library(t, lib)` | Link against a library by name (`-l`) |
| `link_system_library(t, name)` | Link a system library (pkg-config with `-l` fallback) |

### Build Context

| Function | Description |
|---|---|
| `get_toolchain(b)` | Return the detected toolchain descriptor |
| `get_os(b)` | Get the host operating system |
| `get_arch(b)` | Get the host CPU architecture |
| `set_install_prefix(b, prefix)` | Set the installation prefix (default `/usr/local`) |
| `option_bool(b, name, default)` | Read a boolean `-Dname=true\|false` option |
| `option_str(b, name, default)` | Read a string `-Dname=value` option |
| `install_header(b, path)` | Queue a header for installation into `PREFIX/include` |

### Custom Build Steps

| Function | Description |
|---|---|
| `add_step(b, name, fn)` | Register a custom build step with a callback |
| `step_depends_on(step, dep)` | Declare a dependency between two steps |
| `step_set_userdata(step, data)` | Attach user data to a step |
| `step_get_userdata(step)` | Retrieve user data from a step |

### Optimization Levels

| Constant | Compiler Flags |
|---|---|
| `LOOM_OPT_NONE` | `-O0` |
| `LOOM_OPT_DEBUG` | `-Og -g` |
| `LOOM_OPT_RELEASE_SAFE` | `-O2 -g` |
| `LOOM_OPT_RELEASE_FAST` | `-O3` |
| `LOOM_OPT_RELEASE_SMALL` | `-Os` |

## Examples

The [`examples/`](examples/) directory contains self-contained projects demonstrating each feature:

| Example | Description |
|---|---|
| [`01_hello`](examples/01_hello) | Minimal "Hello, World" binary |
| [`02_multi_source`](examples/02_multi_source) | Compiling from multiple source files |
| [`03_static_library`](examples/03_static_library) | Building and linking a static library |
| [`04_shared_library`](examples/04_shared_library) | Building and linking a shared library |
| [`05_glob_sources`](examples/05_glob_sources) | Globbing sources with wildcard patterns |
| [`06_options`](examples/06_options) | Using build-time options (`-Dkey=val`) |
| [`07_optimization`](examples/07_optimization) | Configuring optimization levels |
| [`08_system_library`](examples/08_system_library) | Linking system libraries with pkg-config |
| [`09_custom_steps`](examples/09_custom_steps) | Defining custom build steps with dependencies |
| [`10_multi_target`](examples/10_multi_target) | Building multiple targets in one configuration |
| [`11_raylib_shader`](examples/11_raylib_shader) | Real-world example: a raylib shader project |

To run any example:

```bash
cd examples/01_hello
loom build
loom run
```

## Project Structure

```
loom/
├── include/
│   └── loom.h           # Public API header
├── src/
│   ├── main.c           # CLI entry point
│   ├── init.c           # `loom init` / `loom create` commands
│   ├── loom.c           # Core build logic and API implementation
│   ├── runner.c          # Build runner (compiled build.c executor)
│   ├── toolchain.c      # Compiler auto-detection
│   ├── cache.c          # Build cache (incremental rebuild)
│   ├── hash.c           # File hashing for change detection
│   ├── exec.c           # Process execution utilities
│   ├── path.c           # Path manipulation and globbing
│   └── internal.h       # Private shared declarations
├── examples/            # Self-contained example projects
├── build.c              # Loom's own build configuration (self-hosting)
├── bootstrap.sh         # Bootstrap script for Unix systems
├── bootstrap.ps1        # Bootstrap script for Windows
├── Makefile             # Alternative bootstrap via Make
└── LICENSE              # MIT License
```

## How It Works

Loom follows a simple two-stage process:

1. **Compile the build runner** — Your `build.c` is compiled against `libloom.a` to produce a temporary runner binary (cached in `.loom/`).

2. **Execute the build plan** — The runner calls your `build()` function to collect target definitions, then invokes the detected compiler to build each target respecting the dependency graph.

The runner is cached and only recompiled when `build.c` changes, making subsequent builds near-instant.

## Contributing

Contributions are welcome. If you wish to contribute:

1. Fork the repository.
2. Create a new branch for your feature or fix.
3. Commit and push your changes.
4. Open a Pull Request.

Please format your code with `loom fmt` before submitting.

## License

This project is distributed under the [MIT License](LICENSE).

Copyright © 2026 Elias Dinar Ettouizi.