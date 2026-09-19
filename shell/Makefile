CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -Wpedantic -g -D_POSIX_C_SOURCE=200809L -I../driver
TARGET  := kshell
SRCS    := main.c parser.c exec.c builtins.c note.c
OBJS    := $(SRCS:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)
