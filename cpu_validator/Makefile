CC = gcc
CFLAGS = -std=c11 -Wall -g

all: cpu_validator

cpu_validator: main.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -rf cpu_validator *.o *.dSYM

.PHONY: all test clean
