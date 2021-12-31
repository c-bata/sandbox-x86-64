SRCS = $(wildcard *.c)
HEADS = $(wildcard *.h)
ASMS = $(wildcard test/*.asm)
BINS = $(ASMS:.asm=.bin)

CC = gcc
ASM = nasm
CFLAGS = -std=c11 -Wall -g

all: cpu $(BINS)

cpu: $(SRCS) $(HEADS)
	$(CC) $(CFLAGS) -o $@ $(SRCS)

%.bin: %.asm
	$(ASM) -f bin -o $@ $<

test: all
	./run-tests.sh

clean:
	rm -rf cpu *.o test/*.o test/*.txt test/*.bin *.dSYM

.PHONY: all cpu clean test
