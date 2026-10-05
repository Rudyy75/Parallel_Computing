CC ?= clang
CPPFLAGS ?= -Iinclude
CFLAGS ?= -O2 -Wall -Wextra -std=c11
LDFLAGS ?= -L/system/lib64
LDLIBS ?= -lEGL -lGLESv2

TARGETS = egl_probe forest_fire
NDK ?= $(ANDROID_NDK_HOME)
API ?= 24
HOST_TAG ?= linux-x86_64
TOOLCHAIN ?= $(NDK)/toolchains/llvm/prebuilt/$(HOST_TAG)
CROSS_CC ?= $(TOOLCHAIN)/bin/aarch64-linux-android$(API)-clang
SYSROOT ?= $(TOOLCHAIN)/sysroot

.PHONY: all clean cross

all: $(TARGETS)

egl_probe: egl_probe.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDLIBS)

forest_fire: forest_fire.c forest_fire.comp
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ forest_fire.c $(LDLIBS)

cross:
	$(MAKE) clean
	$(MAKE) CC="$(CROSS_CC)" CPPFLAGS="$(CPPFLAGS) --sysroot=$(SYSROOT)" \
		CFLAGS="$(CFLAGS)" LDFLAGS="--sysroot=$(SYSROOT)" \
		LDLIBS="-lEGL -lGLESv3" all

clean:
	rm -f $(TARGETS)
