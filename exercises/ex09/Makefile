XENO_CONFIG := /usr/xenomai/bin/xeno-config

CFLAGS := $(shell $(XENO_CONFIG)   --posix --alchemy --cflags)
LDFLAGS := $(shell $(XENO_CONFIG)  --posix --alchemy --ldflags)

CC := gcc
EXECUTABLE := ex09a ex09b ex09c ex09d

all: $(EXECUTABLE)

%: %.c
	$(CC) -g -o $@ $< $(CFLAGS) $(LDFLAGS)

clean:
	rm -f $(EXECUTABLE)
