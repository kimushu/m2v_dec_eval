//================================================================================
// m2v_dec_eval - MPEG2 動き補償
// ref: ISO13818-2
// $Id: mc.c 98 2012-04-07 03:52:11Z kimu_shu $
//================================================================================

#include "m2v_dec.h"
#include "dump.h"
#include <malloc.h>
#include <string.h>

// xxx[r][s][t]      r: [0] First vector in MB      [1] Second vector in MB  (*)
//                   s: [0] Forward motion vector   [1] Backward motion vector
//                   t: [0] Horizontal component    [1] Vertical component
//
// (*) DualPrime 利用時は r:[2], [3] もある。
//
// フレーム補償のみ、Bピクチャ不使用のため、r,s は 0 しか取らないので
// 配列サイズは [1][1][2] としている。
int mo_code[1][1][2], mo_residual[1][1][2], mv[1][1][2], PMV[1][1][2];

static uint8_t* frame_buf[2];
static int frame_nowidx;
static uint8_t* frame_now;
static uint8_t* frame_prev;
static int frame_wd, frame_hwd, frame_ht, frame_hht;

// 次のマクロで、x,y座標からのアドレス計算ならびに必要サイズの試算を行う
#define SIZE_CHECK(cx,cy)	((cx) <= 320)
#define BUF_SIZE(cx,cy)		(512*((cy+1)&~1))
#define XY2ADDR_Y(y, x)		(((y)<<9)+(x))
#define XY2ADDR_U(y, x)		(((y)<<10)+(x)+320)
#define XY2ADDR_V(y, x)		(((y)<<10)+(x)+320+512)

#define MBX_WIDTH			6
#define MBY_WIDTH			5
#define REQUIRED_WIDTH		(MBX_WIDTH + MBY_WIDTH + 10)

#define MBX_MASK			((1 << MBX_WIDTH) - 1)
#define MBY_MASK			((1 << MBY_WIDTH) - 1)

#define FBAGEN(b, f, mbx, x2, mby, y)	( \
	( ((f) & 1) << (MBX_WIDTH + MBY_WIDTH + 9) ) | \
	( ((b) & 4) << (MBX_WIDTH + MBY_WIDTH + 6) ) | \
	( ((mby) & MBY_MASK) << (MBX_WIDTH + 8) ) | \
	( ((mbx) & MBX_MASK) << 8 ) | \
	( ((y) & 14) << 4 ) | \
	( (((b) & 4) ? 0 : ((y) & 1)) << 4 ) | \
	( ((x2) & 6) << 1 ) | \
	( (((b) & 4) ? ((b) & 1) : ((x2) & 1)) << 1 ) \
	)

static uint8_t fptr[2][1 << MBY_WIDTH][1 << MBX_WIDTH];
static uint8_t fbuf[1 << REQUIRED_WIDTH];

////////////////////////////////////////////////////////////////////////////////
/// @brief 動き補償 (小ブロック単位)
///
/// TODO: メモリアクセスの出力
///
/// @param SF  [in] 画素差分データ
/// @param b   [in] ブロック番号
///
int mc(int SF[8][8], int b)
{
	int p[8][8];
	int mixf = 1 - (fptr[0][mb_y][mb_x] & 1);

	dump_header(dump_mv);
	dump(dump_mv, "mbx mby intra pat", " %2d %2d %d %d",
			mb_x, mb_y, mb_intra, (mb_pattern >> (5 - b)) & 1);

	if(!mb_mo_fw) mv[0][0][0] = mv[0][0][1] = 0;

	if(mb_intra)
	{
		dumpx_mc_fetch("# no fetch (mb_intra=1)\n");
	}
	else
	{
		uint32_t fetch_a[9][5];
		uint8_t fetch_d[9][10];

		int ivx = mv[0][0][0];
		int ivy = mv[0][0][1];
		dump(dump_mv, "mvx mvy", " %5d %5d", ivx, ivy);

		if(b & 4) { ivx /= 2; ivy /= 2; }	// 絶対値の小さい方へ丸め
		int halfx = ivx & 1;
		int halfy = ivy & 1;
		ivx = (ivx & ~1) / 2;
		ivy = (ivy & ~1) / 2;
		int oddx = ivx & 1;

		dump(dump_mv, "ivx halfx", " %3d %d", ivx, halfx);
		dump(dump_mv, "ivy halfy", " %3d %d", ivy, halfy);

		ivx += (b < 4) ? (mb_x * 16 + (b & 1) * 8) : (mb_x * 8);
		ivy += (b < 4) ? (mb_y * 16 + (b & 2) * 4) : (mb_y * 8);
		dump(dump_mv, "ox oy", " %3d %3d", ivx, ivy);

		dumpx_mc_fetch("# ivx=%d,halfx=%d,ivy=%d,halfy=%d\n",
			ivx, halfx, ivy, halfy);

		// フェッチアドレス計算
		if(b < 4)
		{
			for(int y = 0; y < 9; ++y) for(int x = 0; x < 10; x += 2)
			{
				int ivx2 = x + ivx;
				int ivy2 = y + ivy;
				int f = fptr[0][(ivy2 >> 4) & MBY_MASK][(ivx2 >> 4) & MBX_MASK] & 1;
				fetch_a[y][x >> 1] =
					FBAGEN(b, f, (ivx2 >> 4), (ivx2 & 14) >> 1, (ivy2 >> 4), (ivy2 & 15));
			}
		}
		else
		{
			for(int y = 0; y < 9; ++y) for(int x = 0; x < 10; x += 2)
			{
				int ivx2 = x + ivx;
				int ivy2 = y + ivy;
				int f = fptr[0][(ivy2 >> 3) & MBY_MASK][(ivx2 >> 3) & MBX_MASK] & 1;
				fetch_a[y][x >> 1] =
					FBAGEN(b, f, (ivx2 >> 3), (ivx2 & 14), (ivy2 >> 3), (ivy2 & 7) << 1);
			}
		}

		// フェッチ実行
		for(int y = 0; y < 9; ++y) for(int x = 0; x < 10; x += 2)
		{
			uint32_t a = fetch_a[y][x >> 1];
			fetch_d[y][x + 0] = fbuf[a + 0];
			fetch_d[y][x + 1] = fbuf[a + 1];
			dumpx_mc_fetch("%06x %02x%02x\n",
				fetch_a[y][x >> 1], fetch_d[y][x + 1], fetch_d[y][x + 0]);
		}

		// 整数画素・半画素合成
		for(int y = 0; y < 8; ++y) for(int x = 0; x < 8; ++x)
		{
			int ym = y + halfy;
			p[y][x] = (fetch_d[y][x + oddx] + fetch_d[y][x + oddx + halfx] +
						fetch_d[ym][x + oddx] + fetch_d[ym][x + oddx + halfx] + 2) / 4;

			dumpx_mc_fetch("%s %2x%s",
				x == 0 ? "#" : "", p[y][x], x == 7 ? "\n" : "");
		}
		dumpx_mc_fetch("#------------------------\n");
		// memset(p, 0, sizeof(p));
	}

	// ミックス実行
	for(int y = 0; y < 8; ++y) for(int x = 0; x < 8; ++x)
	{
		uint32_t a;
		if(b < 4)
			a = FBAGEN(b, mixf, mb_x, (x + (b & 1) * 8) >> 1, mb_y, y + (b & 2) * 4);
		else
			a = FBAGEN(b, mixf, mb_x, x, mb_y, y * 2);

		int px = SF[y][x] + (mb_intra ? 0 : p[y][x]);
		if(px < 0)
			px = 0;
		else if(px > 255)
			px = 255;

		fbuf[a + (x & 1)] = px;

		if(x & 1)
		{
			dumpx_mc_mix("%06x %02x%02x\n", a, fbuf[a + 1], fbuf[a + 0]);
		}
	}

	// フレームポインタ更新
	fptr[1][mb_y][mb_x] = 2 | mixf;

	return 1;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief 動きベクトル予測のリセット
///
void mc_reset_mv_pred()
{
	memset(PMV, 0, sizeof(PMV));
}

////////////////////////////////////////////////////////////////////////////////
/// @brief 動きベクトルのデコード
///
/// ビットストリームから読まれた code, residual と現在の予測値を使って
/// 動きベクトルを構築する。予測値の更新も行う。動きベクトルを持つ MB のみで
/// 呼び出すこと。
///
int mc_decode_mv()
{
	const int r = 0, s = 0;

	for(int t = 0; t < 2; ++t)
	{
		int r_size = f_code[s][t] - 1;
		if(r_size == 14) continue;
		int f = 1 << r_size;
		int high = (16 * f) - 1;
		int low = (-16 * f);
		int range = 32 * f;
		int delta;
		int code = mo_code[r][s][t];

		if(f == 1 || code == 0)
			delta = code;
		else if(code > 0)
			delta = ((code - 1) * f) + mo_residual[r][s][t] + 1;
		else
			delta = ((code + 1) * f) - mo_residual[r][s][t] - 1;

		if(delta < low || delta > high)
		{
			printf("\nError: delta is out of range: %d [%d:%d]\n",
				delta, low, high);
			return 0;
		}

		int pred = PMV[r][s][t];
		// if(mv_format == MV_FORMAT_FIELD && t == 1 &&
		//	picture_struct == PIC_STRUCT_FRAME)
		// 	pred /= 2;	// not used

		int x = pred + delta;
		if(x < low) x += range;
		if(x > high) x -= range;
		mv[r][s][t] = x;

		// if(mv_format == MV_FORMAT_FIELD && t == 1 &&
		//	picture_struct == PIC_STRUCT_FRAME)
		// 	PMV[r][s][t] = x * 2;	// not used
		// else
			PMV[r][s][t] = x;

		if(PMV[r][s][t] < low || PMV[r][s][t] > high)
		{
			printf("\nError: PMV is out of range: %d [%d:%d]\n",
				PMV[r][s][t], low, high);
			return 0;
		}
	}
	// printf("[MV]  %3d, %3d\n", mv[0][0][0], mv[0][0][1]);
	// printf("[PMV] %3d, %3d\n", PMV[0][0][0], PMV[0][0][1]);

	return 1;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief フレームバッファの確保
///
/// フレームバッファ(2枚分)を確保する。
///
int mc_allocbuffer()
{
	mc_freebuffer();
	frame_wd = mb_wd << 4;
	frame_hwd = mb_wd << 3;
	frame_ht = mb_ht << 4;
	frame_hht = mb_ht << 3;
	frame_nowidx = 0;
	if(!SIZE_CHECK(frame_wd, frame_ht))
	{
		printf("Error: Frame size is not supported (%dx%d)\n", frame_wd, frame_ht);
		return 0;
	}

	size_t s = BUF_SIZE(frame_wd, frame_ht);
	frame_buf[0] = (uint8_t*)malloc(s * 2);
	if(!frame_buf[0])
	{
		printf("Error: Not enough memory!\n");
		return 0;
	}
	frame_buf[1] = frame_buf[0] + s;
	frame_now = frame_buf[0];
	frame_prev = frame_buf[1];
	return 1;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief フレームバッファの解放
///
void mc_freebuffer()
{
	free(frame_buf[0]);
	frame_buf[0] = frame_buf[1] = NULL;
	frame_now = frame_prev = NULL;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief フレームバッファの切り替え
///
/// フレームバッファのうち、現在の書き込み対象(now)と前フレーム(prev)をスワップ
/// して切り替える。
///
void mc_switchbuffer()
{
	frame_nowidx ^= 1;
	frame_now = frame_buf[frame_nowidx];
	frame_prev = frame_buf[frame_nowidx ^ 1];
}

////////////////////////////////////////////////////////////////////////////////
/// @brief フレームバッファのコピー(MBスキップ用)
///
/// 前フレームから現フレームにコピーする。
///
/// @param mbx MBx座標
/// @param mby MBy座標
/// @param b   対象ブロック
///
int mc_copybuffer(int mbx, int mby, int b)
{
	int bx = mbx << 3;
	int by = mby << 3;
	int cc = (b & 1) + 1;
	if(b < 4)
	{
		bx = (bx << 1) + ((b & 1) << 3);
		by = (by << 1) + ((b & 2) << 2);
		cc = 0;
	}

	if(b < 4)
	{
		for(int y = 0, oy = by; y < 8; ++y, ++oy) for(int x = 0, ox = bx; x < 8; ++x, ++ox)
			mc_memwrite(ox, oy, 0, frame_prev[XY2ADDR_Y(oy, ox)]);
	}
	else if(b == 4)
	{
		for(int y = 0, oy = by; y < 8; ++y, ++oy) for(int x = 0, ox = bx; x < 8; ++x, ++ox)
			mc_memwrite(ox, oy, 1, frame_prev[XY2ADDR_U(oy, ox)]);
	}
	else
	{
		for(int y = 0, oy = by; y < 8; ++y, ++oy) for(int x = 0, ox = bx; x < 8; ++x, ++ox)
			mc_memwrite(ox, oy, 2, frame_prev[XY2ADDR_V(oy, ox)]);
	}

	return 1;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief フレームバッファの YUV 出力
///
/// 現在のフレームバッファを YUV に出力する。
///
void mc_output_yuv()
{
	// フレームポインタの全転送
	for(int mby = 0; mby < (1 << MBY_WIDTH); ++mby)
	{
		for(int mbx = 0; mbx < (1 << MBX_WIDTH); ++mbx)
		{
			fptr[0][mby][mbx] = fptr[1][mby][mbx];
			fptr[1][mby][mbx] &= ~2;
		}
	}

	for(int y = 0; y < video_ht; ++y) for(int x = 0; x < video_wd; x += 2)
	{
		// int v = frame_now[XY2ADDR_Y(y, x)];
		// int i = v * 224 / 256 + 16;
		// if(i > 255) i = 255;
		// int i = v;
		uint32_t a = FBAGEN(0, fptr[0][(y >> 4) & MBY_MASK][(x >> 4) & MBX_MASK],
							x >> 4, x >> 1, y >> 4, y);
		uint8_t i[2] = { fbuf[a + 0], fbuf[a + 1] };
		dumpbin(dump_raw, i, 2);
	}

	int hwd = (video_wd + 1) >> 1;
	int hht = (video_ht + 1) >> 1;
	for(int y = 0; y < hht; ++y) for(int x = 0; x < hwd; x += 2)
	{
		// int v = frame_now[XY2ADDR_U(y, x)];
		// int i = v * 219 / 256 + 16;
		// if(i > 255) i = 255;
		// int i = v;
		uint32_t a = FBAGEN(4, fptr[0][(y >> 3) & MBY_MASK][(x >> 3) & MBX_MASK],
							x >> 3, x, y >> 3, y << 1);
		uint8_t i[2] = { fbuf[a + 0], fbuf[a + 1] };
		dumpbin(dump_raw, i, 2);
	}
	for(int y = 0; y < hht; ++y) for(int x = 0; x < hwd; x += 2)
	{
		// int v = frame_now[XY2ADDR_V(y, x)];
		// int i = v * 219 / 256 + 16;
		// if(i > 255) i = 255;
		// int i = v;
		uint32_t a = FBAGEN(5, fptr[0][(y >> 3) & MBY_MASK][(x >> 3) & MBX_MASK],
							x >> 3, x, y >> 3, y << 1);
		uint8_t i[2] = { fbuf[a + 0], fbuf[a + 1] };
		dumpbin(dump_raw, i, 2);
	}

	if(dump_raw) fflush(dump_raw);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief フレームバッファに書き込み
///
/// 現在のフレームに対し、書き込みを行う。
///
/// @param x     x座標
/// @param y     y座標
/// @param cc    色空間(0:Y,1:U,2:V)
/// @param value 値(0～255)
///
int mc_memwrite(int x, int y, int cc, int value)
{
	if(x < 0 || y < 0 || (cc && (x >= frame_hwd || y >= frame_hht)) ||
		(!cc && (x >= frame_wd || y >= frame_ht)))
	{
		printf("\nError: MC coord is out of range (%d, %d)\n", x, y);
		return 0;
	}

	if(cc == 0)
		frame_now[XY2ADDR_Y(y, x)] = (uint8_t)value;
	else if(cc == 1)
		frame_now[XY2ADDR_U(y, x)] = (uint8_t)value;
	else
		frame_now[XY2ADDR_V(y, x)] = (uint8_t)value;

	return 1;
}

