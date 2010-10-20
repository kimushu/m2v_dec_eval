#================================================================================
# m1v_dec_eval - make
# $Id$
#================================================================================

TARGET = m1vdec

SOURCES = main.c ps.c bitstream.c video.c vlc.c dump.c

CC = gcc
CFLAGS = -Wall -std=c99

OBJS = $(SOURCES:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	@rm -f $(OBJS) $(TARGET)

