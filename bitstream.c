//================================================================================
// m1v_dec_eval - ビットストリーム
// $Id$
//================================================================================

#include <stdio.h>
#include "bitstream.h"

static FILE* fp;
static uint32_t buf, buf2;
static int rest;		// buf内の残り

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
	return NULL;
}

//--------------------------------------------------------------------------------
// get bits (0-32 bits)
//
uint32_t bs_get(int len)
{
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

