//================================================================================
// ビットリーダ
// $Id$
//================================================================================

#ifndef _BITREADER_H_
#define _BITREADER_H_

#include <stdio.h>
#include <stdint.h>

// structures
typedef struct bitstream
{
	uint32_t buf[2];
	FILE* fp;
	int left_bits;
}
bitstream;

// macros
#define bs_peek(bs, b)		((bs->buf[0]) >> (32 - (b)))
#define bs_aligned(bs, b)	((bs->left_bits % (b)) == 0)

// function prototypes
bitstream* bs_open(const char* path);
void bs_close(bitstream* bs);
uint32_t bs_get(bitstream* bs, int bits);
void bs_align(bitstream* bs, int bits);


#endif	/* !_BITREADER_H_ */

