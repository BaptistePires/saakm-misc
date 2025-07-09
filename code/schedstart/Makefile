OUT=schedstart
CC=gcc
CFLAGS=-Wall -Wextra -O2 -g

${OUT}: schedstart.c schedstart.h
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f ${OUT}