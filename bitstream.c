//================================================================================
// m2v_dec_eval - ビットストリーム
// $Id$
//================================================================================

#include <stdio.h>
#include <stdlib.h>
#include "bitstream.h"

static FILE* fp;
static uint32_t buf, buf2;
static int rest;		// buf内の残り

int g_total_bits;

#define SWAP16(w)	((((w)>>8)&0x00ff)|(((w)<<8)&0xff00))
#define SWAP32(w)	((SWAP16(w)<<16)|(SWAP16(w>>16)))

//--------------------------------------------------------------------------------
// open
//
const char* bs_open(const char* file)
{
	fp = fopen(file, "rb");
	if(!fp) return "cannot open file";
	rest = 0;
	fread(&buf2, 4, 1, fp);
	g_total_bits = 0;
	return NULL;
}

//--------------------------------------------------------------------------------
// get bits (0-32 bits)
//
uint32_t bs_get(int len)
{
	g_total_bits += len;
	uint32_t r = 0;
	while(len > 0)
	{
		if(!rest)
		{
			buf = SWAP32(buf2);
			fread(&buf2, 4, 1, fp);
			rest = 32;
		}

		int i = rest < len ? rest : len;

		r <<= i;
		r |= (buf >> (32 - i));
		buf <<= i;

		rest -= i;
		len -= i;
	}
	return r;
}

//--------------------------------------------------------------------------------
// get bits (0-8 bits)
//
uint8_t bs_gets(int len)
{
	if(len > 8)
	{
		fprintf(stderr, "illegal bs_gets call!\n");
		exit(1);
	}
	return bs_get(len);
}


//--------------------------------------------------------------------------------
// peek bits (0-32 bits)
//
uint32_t bs_peek(int len)
{
	uint32_t l_buf = buf;
	int l_rest = rest;
	uint32_t r = 0;
	while(len > 0)
	{
		if(!l_rest)
		{
			l_buf = SWAP32(buf2);
			l_rest = 32;
		}

		int i = l_rest < len ? l_rest : len;

		r <<= i;
		r |= (l_buf >> (32 - i));
		l_buf <<= i;

		l_rest -= i;
		len -= i;
	}

	return r;
}


//--------------------------------------------------------------------------------
// align bits (8,16,32 bits)
//
uint32_t bs_align(int bits)
{
	if(bits != 8 && bits != 16 && bits != 32)
	{
		fprintf(stderr, "illegal call bs_align\n");
		exit(1);
	}

	return bs_get(rest % bits);
}


//--------------------------------------------------------------------------------
// search next code (1-4 bytes)
//
uint16_t bs_nextcode(uint32_t code, int bytes)
{
	if(bytes < 1 || bytes > 4)
	{
		fprintf(stderr, "illegal code bytes\n");
		exit(1);
	}

	for(uint16_t skipped = 0;; ++skipped)
	{
		if(bs_peek(bytes * 8) == code) return skipped;
		bs_gets(8);
	}
}


//--------------------------------------------------------------------------------
// decode VLC
//
int bs_vlc(const VLC_ENTRY* table)
{
	while(table->clen > 0)
	{
		uint32_t c;
		int len;
		c = bs_peek(len = table->clen);
		for(; table->clen == len; ++table)
		{
			if(table->code != c) continue;
			bs_get(len);
			return table->value;
		}
	}
	fprintf(stderr, "VLC decode error (0b");
	uint32_t c = bs_peek(32);
	for(int i = 0; i < 8; ++i)
	{
		if(i > 0) fprintf(stderr, "_");
		fprintf(stderr, "%d%d%d%d",
			(c >> 31) & 1, (c >> 30) & 1, (c >> 29) & 1, (c >> 28) & 1);
		c <<= 4;
	}
	fprintf(stderr, ")\nposition: 0x%x bytes (+%d bits)\n",
		g_total_bits / 8, g_total_bits % 8);
	exit(1);
	return 0;
}


