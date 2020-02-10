#
# makefile for lecture 2
#

CC     = cc
CFLAGS = -Wall -Wextra -g 

who1: who1.c
	$(CC) $(CFLAGS) -o who1 who1.c

who2: who2.c
	$(CC) $(CFLAGS) -o who2 who2.c

who3: who3.o utmplib.o
	$(CC) $(CFLAGS) -o who3 who3.o utmplib.o

swow: wow.o utmplib.o
	$(CC) $(CFLAGS) -o swow wow.o utmplib.o

bwow: wow.o utmplib.o
	$(CC) $(CFLAGS) -o bwow wow.o utmplib.o	

llcopy: llcopy.c
	$(CC) $(CFLAGS) llcopy.c -o llcopy

llcopy1: llcopy.c
	$(CC) -DBUFFERSIZE=1 $(CFLAGS) llcopy.c -o llcopy1

llcopy-prof: llcopy.c
	$(CC) $(CFLAGS) llcopy.c -pg -o llcopy-prof

utmplib.o: utmplib.c
	$(CC) $(CFLAGS) -c utmplib.c

who3.o: who3.c
	$(CC) $(CFLAGS) -c who3.c

wow.o: wow.c
	$(CC) $(CFLAGS) -c wow.c

clean:
	rm -f who1 who2 who3 llcopy utmplib.o who3.o wow.o
#
# tests if llcopy works like cp
#
copytest: llcopy
	last -20 > data
	llcopy data data.copy
	@if diff data data.copy ; then echo success ; else echo error ; fi
