//================================================================================
// m2v_dec_eval - MPEG2 動き補償
// ref: ISO13818-2
// $Id$
//================================================================================

#include "m2v_dec_eval.h"

static uint8_t* frame_buf;
static int frame_nowidx;
uint8_t* frame_now;
uint8_t* frame_prev;
int frame_wd;

////////////////////////////////////////////////////////////////////////////////
/// @brief 動き補償 (小ブロック単位)
///
/// TODO: メモリアクセスの出力
///
/// @param p 動き補償画素データ(出力)
///
int mc(int p[8][8])
{
}

////////////////////////////////////////////////////////////////////////////////
/// @brief フレームバッファの確保/解放
///
/// フレームバッファ(2枚分)を確保する。なお、幅or高さに0を指定すると解放する。
///
int mc_allocbuffer(int mb_wd, int mb_ht)
{
	if(!mb_wd || !mb_ht)
	{
		free(frame_buf);
		frame_buf[0] = frame_buf[1] = NULL;
		frame_now = frame_prev = NULL;
		return 1;
	}

	frame_wd = mb_wd << 4;
	frame_nowidx = 0;
	frame_buf[0] = (uint8_t*)malloc(frame_wd * (mb_ht << 16) * 2);
	if(!frame_buf[0])
	{
		printf("Error: Not enough memory!\n");
		return 0;
	}
	frame_buf[1] = frame_buf[0] + (frame_wd * (mb_ht << 16));
	frame_now = frame_buf[0];
	frame_prev = frame_buf[1];
	return 1;
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


