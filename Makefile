#================================================================================
# m2v_dec_eval - make
# $Id$
#================================================================================

TARGET = m2vdec

TEST = gs_m2v el_m2v sd_m2v

SOURCES = main.c ps.c bitstream.c video.c vlc.c dump.c simple_idct.c ffmpeg_idct.c idct_hw.c

CC = gcc
CFLAGS = -Wall -std=c99 -lm -g

OBJS = $(SOURCES:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c dump.h
	$(CC) $(CFLAGS) -c -o $@ $<

dump.c dump.h: dump.rb
	ruby $<

simple_idct.c: simple_idct.rb
	ruby $< > $@

clean:
	@rm -f $(OBJS) $(TARGET) dump.{c,h} simple_idct.c

.PHONY: test
test: $(foreach t,$(TEST),ref_$(t).mpg.vs/mb.txt)

ref_%/mb.txt: % $(TARGET)
	./$(TARGET) -v $< -s 60 -r > $<.log

