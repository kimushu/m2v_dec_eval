#================================================================================
# m1v_dec_eval - PS デコーダ
# ref: ISO13818-1
# $Id$
#================================================================================

TARGET = m1vdec

SOURCES = ps.c

CC = gcc
CFLAGS = -Wall -std=c99

OBJS = $(SOURCES:.c=.o)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

