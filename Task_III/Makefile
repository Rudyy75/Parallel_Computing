CC ?= clang
CPPFLAGS ?= -Iinclude
CFLAGS ?= -O2 -Wall -Wextra -std=c11
LDFLAGS ?= -L/system/lib64
LDLIBS ?= -lEGL -lGLESv2

TARGET = egl_probe

.PHONY: all clean

all: $(TARGET)

$(TARGET): egl_probe.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDLIBS)

clean:
	rm -f $(TARGET)
