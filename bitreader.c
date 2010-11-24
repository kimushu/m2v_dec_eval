//================================================================================
// ビットリーダ
// $Id$
//================================================================================

#include "bitreader.h"

////////////////////////////////////////////////////////////////////////////////
/// @brief ビットストリームを開く
///
/// ビットストリームを開き、アクセス用ポインタを返す。
///
/// @param path ファイルのパス
///
bitstream* bs_open(const char* path)
{
	FILE* fp = fopen(path, "rb");
	if(!fp)
	{
		printf("Error: Cannot open file '%s'.\n", path);
		return NULL;
	}
	bitstream* bs = (bitstream*)malloc(sizeof(bitstream));
	bs->buf[0] = bs->buf[1] = 0;
	if(fread(bs->buf, 4, 2, fp) < 2)
		bs->left_bits = -1;
	else
		bs->left_bits = 32;

	bs->fp = fp;
	return bs;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief ビットストリームを閉じる
///
void bs_close(bitstream* bs)
{
	fclose(bs->fp);
	free(bs);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief ビットを読み込み
///
uint32_t bs_get(bitstream* bs, int bits)
{
	uint32_t r = bs_peek(bs, bits);
	while(bits > 0)
	{
		int b = bs->left_bits;
		if(bits < b)
		{
			bs->buf[0] = (bs->buf[0] << bits) | (bs->buf[1] >> (32 - bits));
			bs->buf[1] <<= (32 - bits);
			break;
		}
		else if(b < 0)
			break;

		bs->buf[0] = (bs->buf[0] << b) | (bs->buf[1] >> (32 - b));
		if(fread(bs->buf + 1, 4, 1, bs->fp) < 1)
		{
			bs->buf[1] = 0;
			bs->left_bits = -1;
		}
		else bs->left_bits = 32;
		bits -= b;
	}
	return r;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief 境界へ移動
///
void bs_align(bitstream* bs, int bits)
{
	if(!bs_aligned(bs, bits)) bs_get(bs, bs->left_bits % bits);
}


