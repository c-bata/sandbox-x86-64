CC = gcc
SRCS = main.c
CFLAGS = -std=c11 -Wall -g

9cc: $(SRCS)
	$(CC) $(CFLAGS) -o $@ $(SRCS)

test: 9cc
	./run-tests.sh

clean:
	rm -rf 9cc *.o *~ tmp* *.dSYM

.PHONY: 9cc test clean
