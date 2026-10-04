CC ?= clang
CPPFLAGS ?= -Iinclude
CFLAGS ?= -O2 -Wall -Wextra -std=c11
LDFLAGS ?= -L/system/lib64
LDLIBS ?= -lEGL -lGLESv2

TARGETS = egl_probe forest_fire

.PHONY: all clean

all: $(TARGETS)

egl_probe: egl_probe.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDLIBS)

forest_fire: forest_fire.c forest_fire.comp
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ forest_fire.c $(LDLIBS)

clean:
	rm -f $(TARGETS)
