#CC=clang
CC=gcc
CFLAGS=-Wall -Wextra -g
DEVS=ctlr/devices/simple.c
SRCS=ctlr/ctlr.c demo.c
TRGT=demo

all:
	$(CC) $(CFLAGS) $(DEVS) $(SRCS) -o $(TRGT)
