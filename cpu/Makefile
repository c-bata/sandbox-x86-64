SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
HEADS = $(wildcard *.h)
ASMS = $(wildcard test/*.asm)
BINS = $(ASMS:.asm=.bin)

CC = gcc
ASM = nasm
CFLAGS = -std=c11 -Wall -g

all: cpu $(BINS)

cpu: $(OBJS) $(HEADS)
	$(CC) $(CFLAGS) -o $@ $(SRCS)

%.bin: %.asm
	$(ASM) -f bin -o $@ $<

test: all $(OBJS)
	./run-tests.sh

clean:
	rm -rf cpu *.o test/*.o test/*.txt test/*.bin *.dSYM

.PHONY: all test clean
