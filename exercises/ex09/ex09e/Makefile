EXECUTABLE := ex09e

SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

XENO_CONFIG := /usr/xenomai/bin/xeno-config

CFLAGS := $(shell $(XENO_CONFIG)   --posix --alchemy  --cflags) -L.
LDFLAGS := $(shell $(XENO_CONFIG)  --posix --alchemy --ldflags)

#CC := $(shell $(XENO_CONFIG) --cc)
CC := /usr/bin/gcc

.PHONY: all clean run

# what to make by default
all: $(EXECUTABLE)

# make objects
%.o: %.c
	$(CC) -o $*.o -c $*.c $(CFLAGS)

# link objects
$(EXECUTABLE):  $(OBJS)
	$(CC) -o $(EXECUTABLE) $(OBJS) $(LDFLAGS)

# tools
clean:
	rm -f *.o $(EXECUTABLE)

run: $(EXECUTABLE)
	./$(EXECUTABLE)

