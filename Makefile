#================================================================================
# m2v_dec_eval - make
# $Id: Makefile 98 2012-04-07 03:52:11Z kimu_shu $
#================================================================================

TARGET = m2vdec

# TEST = gs_m2v el_m2v sd_m2v el1m_m2v sd1m_m2v
#TEST ?= el1m_m2v nichi2m_m2v
#TEST ?= el1m_m2v
#TEST ?= mj1m_m2v pd1m_m2v
TEST ?= pdcut

SOURCES = video.c ps.c vlc.c dump.c bitreader.c bitdecoder.c dequant.c \
			idct.c mc.c

CC = gcc
CFLAGS = -Wall -std=gnu99 -lm -g

OBJDIR = .obj
OBJS = $(addprefix $(OBJDIR)/,$(SOURCES:.c=.o))

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(OBJDIR)/%.o: %.c dump.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

dump.c dump.h: dump.rb
	ruby $<

clean:
	@rm -f $(OBJS) $(TARGET) dump.{c,h}

.PHONY: test
test: $(foreach t,$(TEST),ref_$(t).mpg.vs/mb.txt)

ref_%/mb.txt: % $(TARGET)
	./$(TARGET) -i $< -T 60 `cat $*.opt` -d
	ln -sf ../$* ref_$*/input.bin

