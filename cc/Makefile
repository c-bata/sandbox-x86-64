CC = gcc
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
CFLAGS = -std=c11 -Wall -g

9cc: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

tokendump: tools/tokendump.c tokenize.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJS): 9cc.h

test: 9cc
	./run-tests.sh

clean:
	rm -rf 9cc *.o *~ tmp* *.dSYM

.PHONY: test clean
