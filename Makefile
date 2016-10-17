#CC=clang
CC=gcc

all:
	$(CC) -Wall -Wextra ctlr/devices/simple.c ctlr/devices/ni_maschine.c  ctlr/ctlr.c demo.c -o demo
