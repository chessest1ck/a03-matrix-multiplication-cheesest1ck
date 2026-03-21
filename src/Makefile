#
# Makefile that builds btest and other helper programs for the CS:APP data lab
# 
CC = gcc
CFLAGS = -O -Wall -pthread

all: mm-mt

mm: mm-mt.c mm.h 
	$(CC) $(CFLAGS) -o mm mm.c 

mm-mt: mm-mt.c mm-mt.h
	$(CC) $(CFLAGS) -o mm-mt mm-mt.c

clean:
	rm -f *.o mm
