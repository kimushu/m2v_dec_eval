#================================================================================
# m1v_dec_eval - make
# $Id$
#================================================================================

TARGET = m1vdec

SOURCES = main.c ps.c bitstream.c

CC = gcc
CFLAGS = -Wall -std=c99

OBJS = $(SOURCES:.c=.o)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

