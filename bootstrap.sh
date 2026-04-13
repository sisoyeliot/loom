#!/bin/sh
set -e

CC="${CC:-cc}"
AR="${AR:-ar}"
PREFIX="${PREFIX:-/usr/local}"
CFLAGS="-std=c11 -Wall -Wextra -O2 -Iinclude"

echo "Bootstrapping loom..."
echo "  CC     = $CC"
echo "  AR     = $AR"
echo "  PREFIX = $PREFIX"
echo ""

mkdir -p build/cli build/lib

for f in src/main.c src/init.c src/path.c src/exec.c; do
    echo "  CC   $f"
    $CC $CFLAGS -DLOOM_PREFIX=\""$PREFIX"\" -c "$f" -o "build/cli/$(basename "$f" .c).o"
done

echo "  LINK build/loom"
$CC -o build/loom build/cli/*.o

for f in src/loom.c src/runner.c src/toolchain.c src/hash.c src/cache.c src/exec.c src/path.c; do
    echo "  CC   $f"
    $CC $CFLAGS -c "$f" -o "build/lib/$(basename "$f" .c).o"
done

echo "  AR   build/libloom.a"
$AR rcs build/libloom.a build/lib/*.o

echo ""
echo "Build complete."
echo "  loom binary  : build/loom"
echo "  loom library : build/libloom.a"
echo ""
echo "To install:"
echo "  sudo make install"
echo ""
echo "After installation loom can rebuild itself:"
echo "  loom build"
echo "  loom install -Dprefix=\$HOME/.local"
