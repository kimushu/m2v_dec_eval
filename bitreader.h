//================================================================================
// ビットリーダ
// $Id: bitreader.h 98 2012-04-07 03:52:11Z kimu_shu $
//================================================================================

#ifndef _BITREADER_H_
#define _BITREADER_H_

#include <stdio.h>
#include <stdint.h>
#include "vlc.h"

// structures
typedef struct bitstream
{
	uint32_t buf[2];
	FILE* fp;
	int left_bits;
	int last_vlclen;
}
bitstream;

// macros
#define bs_peek(bs, b)		((bs->buf[0]) >> (32 - (b)))
#define bs_aligned(bs, b)	((bs->left_bits % (b)) == 0)
#define bs_vlclen(bs)		(bs->last_vlclen)

// function prototypes
bitstream* bs_open(const char* path);
void bs_close(bitstream* bs);
uint32_t bs_get(bitstream* bs, int bits, const char* dump);
void bs_align(bitstream* bs, int bits);
int bs_vlc(bitstream* bs, const VLC_TABLE* t, const char* dump);


#endif	/* !_BITREADER_H_ */

