CC      ?= cc
AR      ?= ar
PREFIX  ?= /usr/local
CFLAGS   = -std=c11 -Wall -Wextra -O2 -Iinclude

CLI_SRC  = src/main.c src/init.c src/path.c
LIB_SRC  = src/loom.c src/runner.c src/toolchain.c src/hash.c \
           src/cache.c src/exec.c src/path.c

CLI_OBJ  = $(CLI_SRC:src/%.c=build/cli/%.o)
LIB_OBJ  = $(LIB_SRC:src/%.c=build/lib/%.o)

all: build/loom build/libloom.a

build/loom: $(CLI_OBJ)
	@mkdir -p build
	@printf "  LINK\t$@\n"
	@$(CC) -o $@ $^

build/libloom.a: $(LIB_OBJ)
	@mkdir -p build
	@printf "  AR\t$@\n"
	@$(AR) rcs $@ $^

build/cli/%.o: src/%.c src/internal.h include/loom.h
	@mkdir -p $(dir $@)
	@printf "  CC\t$<\n"
	@$(CC) $(CFLAGS) -DLOOM_PREFIX=\"$(PREFIX)\" -c $< -o $@

build/lib/%.o: src/%.c src/internal.h include/loom.h
	@mkdir -p $(dir $@)
	@printf "  CC\t$<\n"
	@$(CC) $(CFLAGS) -c $< -o $@

install: all
	install -d $(DESTDIR)$(PREFIX)/bin
	install -d $(DESTDIR)$(PREFIX)/include
	install -d $(DESTDIR)$(PREFIX)/lib
	install -m 755 build/loom $(DESTDIR)$(PREFIX)/bin/loom
	install -m 644 include/loom.h $(DESTDIR)$(PREFIX)/include/loom.h
	install -m 644 build/libloom.a $(DESTDIR)$(PREFIX)/lib/libloom.a

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/loom
	rm -f $(DESTDIR)$(PREFIX)/include/loom.h
	rm -f $(DESTDIR)$(PREFIX)/lib/libloom.a

clean:
	rm -rf build

.PHONY: all install uninstall clean
