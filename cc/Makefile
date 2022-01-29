CC = gcc
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
CFLAGS = -std=c11 -Wall -Wno-switch -Wno-return-type -g

all: 9cc tokendump astdump

9cc: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

tokendump: tools/tokendump.c tokenize.c my_string.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

astdump: tools/astdump.c tokenize.c parse.c type.c my_string.c
	$(CC) $(CFLAGS) -Wno-pointer-to-int-cast -o $@ $^ $(LDFLAGS)

$(OBJS): 9cc.h

test: 9cc
	./tests.sh

clean:
	rm -rf 9cc astdump tokendump *.o *~ tmp* *.dSYM

.PHONY: all test clean
