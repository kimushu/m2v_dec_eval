//================================================================================
// m1v_dec_eval - ビットストリーム
// $Id$
//================================================================================

#include <stdio.h>
#include "bitstream.h"

static FILE* fp;
static uint32_t buf;
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
	return NULL;
}

//--------------------------------------------------------------------------------
// open (0-32 bits)
//
uint32_t bs_get(int len)
{
	uint32_t r = 0;
	while(len > 0)
	{
		if(!rest)
		{
			fread(&buf, 4, 1, fp);
			buf = SWAP32(buf);
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

