CC ?= cc
CFLAGS ?= -O2 -Wall -Wextra -std=c11
LDFLAGS ?= -pthread

TARGET = spsc_ringbuf

.PHONY: all clean

all: $(TARGET)

$(TARGET): spsc_ringbuf.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

clean:
	rm -f $(TARGET)
