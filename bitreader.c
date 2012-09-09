//================================================================================
// ビットリーダ
// $Id: bitreader.c 98 2012-04-07 03:52:11Z kimu_shu $
//================================================================================

#include "bitreader.h"
#include "dump.h"
#include <malloc.h>
#include <string.h>

#define PRINT_BS_GET	0

#define SWAP16(w)	((((w)>>8)&0x00ff)|(((w)<<8)&0xff00))
#define SWAP32(w)	((SWAP16(w)<<16)|(SWAP16(w>>16)))


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

	bs->buf[0] = SWAP32(bs->buf[0]);
	bs->buf[1] = SWAP32(bs->buf[1]);
	// printf("bs_get: next 0x%08x,0x%08x\n", bs->buf[0], bs->buf[1]);
	bs->fp = fp;
	bs->last_vlclen = 0;
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
uint32_t bs_get(bitstream* bs, int bits, const char* dump)
{
	uint32_t r = bits > 0 ? bs_peek(bs, bits) : 0;
	char text[64], *p;
	if(dump)
	{
		uint32_t r2 = (r << (32 - bits));
		int i;
		strcpy(text, dump);
		p = text + strlen(text);
		*(p++) = '\t';
		for(i = bits; i > 0; --i, r2 <<= 1)
		{
			*(p++) = '0' + (r2 >> 31);
		}
		*p = 0;
		// dump_bitstream("%s\n", text);
		dumpf(dump_bitstream, "%s\n", text);
	}
#if(PRINT_BS_GET)
	{
		uint32_t r2 = r;
		printf("(%2d) ", bits);
		for(int i = 0; i < bits; ++i, r2 <<= 1)
		{
			if(i > 0 && !(i % 8)) printf("_");
			printf("%d", (r2 >> (bits - 1)) & 1);
		}
		printf("\n");
	}
#endif
	while(bits > 0)
	{
		int b = bs->left_bits;
		if(bits < b)
		{
			bs->buf[0] = (bs->buf[0] << bits) | (bs->buf[1] >> (32 - bits));
			bs->buf[1] <<= bits;
			bs->left_bits -= bits;
			break;
		}
		else if(b < 0)
			break;

		if(b == 32)
			bs->buf[0] = bs->buf[1];
		else
			bs->buf[0] = (bs->buf[0] << b) | (bs->buf[1] >> (32 - b));

		if(fread(bs->buf + 1, 4, 1, bs->fp) < 1)
		{
			bs->buf[1] = 0;
			bs->left_bits = -1;
		}
		else bs->left_bits = 32;
		bs->buf[1] = SWAP32(bs->buf[1]);
		bits -= b;
	}
	// printf("bs_get: next 0x%08x,0x%08x\n", bs->buf[0], bs->buf[1]);
	return r;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief 境界へ移動
///
void bs_align(bitstream* bs, int bits)
{
	if(!bs_aligned(bs, bits)) bs_get(bs, bs->left_bits % bits, "align");
	else bs_get(bs, 0, "align");
}

////////////////////////////////////////////////////////////////////////////////
/// @brief VLCのデコード
///
/// VLC のデコードを行う。得られた値を返す。該当コードが無かった場合、
/// -99 を返す。
///
int bs_vlc(bitstream* bs, const VLC_TABLE* t, const char* dump)
{
	int* r = t->table + bs_peek(bs, t->bits) * 2;
	bs_get(bs, bs->last_vlclen = r[1], dump);
	return r[0];
}

