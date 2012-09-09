//================================================================================
// m2v_dec_eval - MPEG2 8x8 IDCT
// ref: ISO13818-2
// $Id: idct.c 97 2012-03-15 16:08:32Z kimu_shu $
//================================================================================

#include "m2v_dec.h"
#include "dump.h"
#include <string.h>

static inline void idct_row(int* row);
static inline void idct_col(int* col);

#define TRANSPOSITION	// 行列を転置して計算する
// #define ONLY_4X4		// 低周波領域の4x4のみで行う

////////////////////////////////////////////////////////////////////////////////
/// @brief Inverse DCT の実行 (小ブロック単位)
///
/// @param lf 入力する周波数領域データ([v][u]の順)
/// @param sf 出力される空間領域データ([y][x]の順)
///
int idct(int lf[8][8], int sf[8][8])
{
	//           2  N-1 N-1                    (2x+1)uπ     (2y+1)vπ
	// f(x,y) = --- Σ  Σ  C(u)C(v)F(u,v) cos --------- cos ---------
	//           N  u=0 v=0                        2N            2N
	// (N=8)

	// 1次元 IDCT に分解すると、
	//           1  N-1                 (2y+1)vπ
	// w(u,y) = --- Σ  C(v) F(u,v) cos ---------
	//           2  v=0                     2N
	//
	//           1  N-1                 (2x+1)uπ
	// f(x,y) = --- Σ  C(u) w(u,y) cos ---------
	//           2  u=0                     2N
	//
	// となり、ほぼ同様の計算を二度行えばいいことになる。
	//

	memcpy(sf, lf, sizeof(int) * 64);
	dump_header(dump_idct_mid);
	dump_header(dump_idct_row);
	dump_header(dump_idct_col);

	for(int i = 0; i < 8; ++i)
	{
#ifndef TRANSPOSITION
		dump(dump_idct_row, "row", " %d", i);
		idct_row(sf[0] + i * 8);
#else
		dump(dump_idct_col, "col", " %d", i);
		idct_col(sf[0] + i);
#endif
	}

	for(int i = 0; i < 64; ++i)
		dumpf(dump_idct_mid, " %4d%s", sf[0][i], (i & 7) == 7 ? "\n" : "");

	for(int i = 0; i < 8; ++i)
	{
#ifndef TRANSPOSITION
		dump(dump_idct_col, "col", " %d", i);
		idct_col(sf[0] + i);
#else
		dump(dump_idct_row, "row", " %d", i);
		idct_row(sf[0] + i * 8);
#endif
	}

	// dump_header(dump_idct);
	for(int i = 0; i < 64; ++i)
	{
		int v = sf[0][i];
		if(v < -256)
			sf[0][i] = -256;
		else if(v > 255)
			sf[0][i] = 255;
		dumpf(dump_idct, "%4d%s", sf[0][i], (i & 7) == 7 ? "\n" : "");
		dumpx_idct_out("%4d%s", sf[0][i], (i & 7) == 7 ? "\n" : "");
	}

	return 1;
}

#define W1  22725  //cos(i*M_PI/16)*sqrt(2)*(1<<14) + 0.5
#define W2  21407  //cos(i*M_PI/16)*sqrt(2)*(1<<14) + 0.5
#define W3  19266  //cos(i*M_PI/16)*sqrt(2)*(1<<14) + 0.5
// #define W4  16383  //cos(i*M_PI/16)*sqrt(2)*(1<<14) + 0.5
#define W4  16384  //cos(i*M_PI/16)*sqrt(2)*(1<<14) + 0.5
#define W5  12873  //cos(i*M_PI/16)*sqrt(2)*(1<<14) + 0.5
#define W6  8867   //cos(i*M_PI/16)*sqrt(2)*(1<<14) + 0.5
#define W7  4520   //cos(i*M_PI/16)*sqrt(2)*(1<<14) + 0.5
#ifndef TRANSPOSITION
#define ROW_SHIFT 11
#define COL_SHIFT 20 // 6
#else
#define ROW_SHIFT 20
#define COL_SHIFT 11
#endif

static inline void idct_row(int* row)
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
#ifndef ONLY_4X4
	a0       += ( W4 * row[4]);
	  a1     += (-W4 * row[4]);
	    a2   += (-W4 * row[4]);
	      a3 += ( W4 * row[4]);

	a0       += ( W6 * row[6]);
	  a1     += (-W2 * row[6]);
	    a2   += ( W2 * row[6]);
	      a3 += (-W6 * row[6]);
#endif
	b0       += ( W1 * row[1]);
	  b1     += ( W3 * row[1]);
	    b2   += ( W5 * row[1]);
	      b3 += ( W7 * row[1]);

	b0       += ( W3 * row[3]);
	  b1     += (-W7 * row[3]);
	    b2   += (-W1 * row[3]);
	      b3 += (-W5 * row[3]);
#ifndef ONLY_4X4
	b0       += ( W5 * row[5]);
	  b1     += (-W1 * row[5]);
	    b2   += ( W7 * row[5]);
	      b3 += ( W3 * row[5]);

	b0       += ( W7 * row[7]);
	  b1     += (-W5 * row[7]);
	    b2   += ( W3 * row[7]);
	      b3 += (-W1 * row[7]);
#endif
	dump(dump_idct_row, "a0-a3", " %9d %9d %9d %9d", a0, a1, a2, a3);
	dump(dump_idct_row, "b0-b3", " %9d %9d %9d %9d", b0, b1, b2, b3);
	row[0] = (short)((a0 + b0) >> ROW_SHIFT);
	row[1] = (short)((a1 + b1) >> ROW_SHIFT);
	row[2] = (short)((a2 + b2) >> ROW_SHIFT);
	row[3] = (short)((a3 + b3) >> ROW_SHIFT);
	row[4] = (short)((a3 - b3) >> ROW_SHIFT);
	row[5] = (short)((a2 - b2) >> ROW_SHIFT);
	row[6] = (short)((a1 - b1) >> ROW_SHIFT);
	row[7] = (short)((a0 - b0) >> ROW_SHIFT);
}

static inline void idct_col(int* col)
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
#ifndef ONLY_4X4
	a0       += ( W4 * col[8*4]);
	  a1     += (-W4 * col[8*4]);
	    a2   += (-W4 * col[8*4]);
	      a3 += ( W4 * col[8*4]);

	a0       += ( W6 * col[8*6]);
	  a1     += (-W2 * col[8*6]);
	    a2   += ( W2 * col[8*6]);
	      a3 += (-W6 * col[8*6]);
#endif
	b0       += ( W1 * col[8*1]);
	  b1     += ( W3 * col[8*1]);
	    b2   += ( W5 * col[8*1]);
	      b3 += ( W7 * col[8*1]);

	b0       += ( W3 * col[8*3]);
	  b1     += (-W7 * col[8*3]);
	    b2   += (-W1 * col[8*3]);
	      b3 += (-W5 * col[8*3]);
#ifndef ONLY_4X4
	b0       += ( W5 * col[8*5]);
	  b1     += (-W1 * col[8*5]);
	    b2   += ( W7 * col[8*5]);
	      b3 += ( W3 * col[8*5]);

	b0       += ( W7 * col[8*7]);
	  b1     += (-W5 * col[8*7]);
	    b2   += ( W3 * col[8*7]);
	      b3 += (-W1 * col[8*7]);
#endif
	dump(dump_idct_col, "a0-a3", " %9d %9d %9d %9d", a0, a1, a2, a3);
	dump(dump_idct_col, "b0-b3", " %9d %9d %9d %9d", b0, b1, b2, b3);
	col[8*0] = (short)((a0 + b0) >> COL_SHIFT);
	col[8*1] = (short)((a1 + b1) >> COL_SHIFT);
	col[8*2] = (short)((a2 + b2) >> COL_SHIFT);
	col[8*3] = (short)((a3 + b3) >> COL_SHIFT);
	col[8*4] = (short)((a3 - b3) >> COL_SHIFT);
	col[8*5] = (short)((a2 - b2) >> COL_SHIFT);
	col[8*6] = (short)((a1 - b1) >> COL_SHIFT);
	col[8*7] = (short)((a0 - b0) >> COL_SHIFT);
}

