#include <stdint.h>
#include "dump.h"

// ffmpeg をベースにしてます。
// ベースというよりそのまんまですが ^^;

#define W1  22725  //cos(i*M_PI/16)*sqrt(2)*(1<<14) + 0.5
#define W2  21407  //cos(i*M_PI/16)*sqrt(2)*(1<<14) + 0.5
#define W3  19266  //cos(i*M_PI/16)*sqrt(2)*(1<<14) + 0.5
#define W4  16383  //cos(i*M_PI/16)*sqrt(2)*(1<<14) + 0.5
#define W5  12873  //cos(i*M_PI/16)*sqrt(2)*(1<<14) + 0.5
#define W6  8867   //cos(i*M_PI/16)*sqrt(2)*(1<<14) + 0.5
#define W7  4520   //cos(i*M_PI/16)*sqrt(2)*(1<<14) + 0.5
#define ROW_SHIFT 11
#define COL_SHIFT 20 // 6

typedef short elem_t;

static inline void idct_row(elem_t* row)
{
	int a0, a1, a2, a3, b0, b1, b2, b3;

	a0 = a1 = a2 = a3 = (1 << (ROW_SHIFT - 1));
	b0 = b1 = b2 = b3 = 0;

	a0       += ( W4 * row[0]);
	  a1     += ( W4 * row[0]);
	    a2   += ( W4 * row[0]);
	      a3 += ( W4 * row[0]);

	a0       += ( W2 * row[2]);
	  a1     += ( W6 * row[2]);
	    a2   += (-W6 * row[2]);
	      a3 += (-W2 * row[2]);

	a0       += ( W4 * row[4]);
	  a1     += (-W4 * row[4]);
	    a2   += (-W4 * row[4]);
	      a3 += ( W4 * row[4]);

	a0       += ( W6 * row[6]);
	  a1     += (-W2 * row[6]);
	    a2   += ( W2 * row[6]);
	      a3 += (-W6 * row[6]);

	b0       += ( W1 * row[1]);
	  b1     += ( W3 * row[1]);
	    b2   += ( W5 * row[1]);
	      b3 += ( W7 * row[1]);

	b0       += ( W3 * row[3]);
	  b1     += (-W7 * row[3]);
	    b2   += (-W1 * row[3]);
	      b3 += (-W5 * row[3]);

	b0       += ( W5 * row[5]);
	  b1     += (-W1 * row[5]);
	    b2   += ( W7 * row[5]);
	      b3 += ( W3 * row[5]);

	b0       += ( W7 * row[7]);
	  b1     += (-W5 * row[7]);
	    b2   += ( W3 * row[7]);
	      b3 += (-W1 * row[7]);

	row[0] = (a0 + b0) >> ROW_SHIFT;
	row[1] = (a1 + b1) >> ROW_SHIFT;
	row[2] = (a2 + b2) >> ROW_SHIFT;
	row[3] = (a3 + b3) >> ROW_SHIFT;
	row[4] = (a3 - b3) >> ROW_SHIFT;
	row[5] = (a2 - b2) >> ROW_SHIFT;
	row[6] = (a1 - b1) >> ROW_SHIFT;
	row[7] = (a0 - b0) >> ROW_SHIFT;
}

static inline void idct_col(elem_t* col)
{
	int a0, a1, a2, a3, b0, b1, b2, b3;

	a0 = a1 = a2 = a3 = (1 << (COL_SHIFT - 1));
	b0 = b1 = b2 = b3 = 0;

	a0       += ( W4 * col[8*0]);
	  a1     += ( W4 * col[8*0]);
	    a2   += ( W4 * col[8*0]);
	      a3 += ( W4 * col[8*0]);

	a0       += ( W2 * col[8*2]);
	  a1     += ( W6 * col[8*2]);
	    a2   += (-W6 * col[8*2]);
	      a3 += (-W2 * col[8*2]);

	a0       += ( W4 * col[8*4]);
	  a1     += (-W4 * col[8*4]);
	    a2   += (-W4 * col[8*4]);
	      a3 += ( W4 * col[8*4]);

	a0       += ( W6 * col[8*6]);
	  a1     += (-W2 * col[8*6]);
	    a2   += ( W2 * col[8*6]);
	      a3 += (-W6 * col[8*6]);

	b0       += ( W1 * col[8*1]);
	  b1     += ( W3 * col[8*1]);
	    b2   += ( W5 * col[8*1]);
	      b3 += ( W7 * col[8*1]);

	b0       += ( W3 * col[8*3]);
	  b1     += (-W7 * col[8*3]);
	    b2   += (-W1 * col[8*3]);
	      b3 += (-W5 * col[8*3]);

	b0       += ( W5 * col[8*5]);
	  b1     += (-W1 * col[8*5]);
	    b2   += ( W7 * col[8*5]);
	      b3 += ( W3 * col[8*5]);

	b0       += ( W7 * col[8*7]);
	  b1     += (-W5 * col[8*7]);
	    b2   += ( W3 * col[8*7]);
	      b3 += (-W1 * col[8*7]);

	col[8*0] = (a0 + b0) >> COL_SHIFT;
	col[8*1] = (a1 + b1) >> COL_SHIFT;
	col[8*2] = (a2 + b2) >> COL_SHIFT;
	col[8*3] = (a3 + b3) >> COL_SHIFT;
	col[8*4] = (a3 - b3) >> COL_SHIFT;
	col[8*5] = (a2 - b2) >> COL_SHIFT;
	col[8*6] = (a1 - b1) >> COL_SHIFT;
	col[8*7] = (a0 - b0) >> COL_SHIFT;
}

void idct_hw(elem_t* block)
{
	int i;
	for(i = 0; i < 8; ++i) idct_row(block + i * 8);
	for(i = 0; i < 8 && dump_idwb; ++i)
	{
		int j;
		for(j = 0; j < 8; ++j) fprintf(dump_idwb, " %6d", block[i*8+j]);
		fprintf(dump_idwb, "\n");
	}
	for(i = 0; i < 8; ++i) idct_col(block + i);
}

