CC ?= cc
CFLAGS ?= -O2 -Wall -Wextra -std=c11
LDFLAGS ?= -pthread

TARGET = array_sum

.PHONY: all clean

all: $(TARGET)

$(TARGET): array_sum.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

clean:
	rm -f $(TARGET)
