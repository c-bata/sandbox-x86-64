CC = gcc
CFLAGS = -std=c11 -Wall -g

all: cpu_validator ndb

cpu_validator: main.c
	$(CC) $(CFLAGS) -o $@ $<

ndb: ndb.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -rf cpu_validator *.o *.dSYM

.PHONY: all test clean
