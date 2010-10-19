//================================================================================
// m1v_dec_eval - MPEG1 ビデオ デコーダ
// ref: ISO13818-2?
// $Id$
//================================================================================

#include <stdio.h>
#include "m1vdec.h"
#include "bitstream.h"
#include "video.h"
#include "vlc.h"

static const char* sequence_header();
static const char* gop();
static const char* picture();
static const char* slice();
static const char* macroblock();

int g_v_packet;

int video_wd, video_ht;
int pic_coding_type;
int full_pel_fw_vector, full_pel_bw_vector;
int fw_f_code, bw_f_code;
int quant_scale;
int mb_addr_inc, mb_type;
int mb_quant, mb_mo_fw, mb_mo_bw, mb_pat, mb_intra;

static const char* const pct_string = "0IPBD567";


const char* decode_video()
{
	while(bs_peek(32) == 0x000001b3)
	{
		CALL(sequence_header());
		while(bs_peek(32) == 0x000001b8) CALL(gop());
	}
	if(bs_get(32) != 0x000001b7) return "illegal sequence end code";
	return NULL;
}

static const char* sequence_header()
{
	bs_get(32);
	printf("---- [%06u] SEQUENCE HEADER ----\n", g_v_packet++);
	video_wd = bs_get(12);
	video_ht = bs_get(12);
	printf("size: %d x %d\n", video_wd, video_ht);
	printf("pel_aspect_ratio: %u\n", bs_gets(4));
	printf("picture_rate: %u\n", bs_gets(4));
	printf("bitrate: %u\n", bs_get(18));
	if(bs_gets(1) != 0b1) return "illegal marker-bit (seqh)";
	printf("vbv_buf_size: %u\n", bs_get(10));
	printf("const_param_flag: %u\n", bs_gets(1));
	if(bs_gets(1) == 0b1)
	{
		printf("intra_quant_matrix:");
		for(int i = 0; i < 64; ++i)
		{
			if(i > 0 && !(i & 7)) printf("                   ");
			printf(" %3u", bs_gets(8));
			if((i & 7) == 7) printf("\n");
		}
	}
	if(bs_gets(1) == 0b1)
	{
		printf("non_intra_quant_matrix:");
		for(int i = 0; i < 64; ++i)
		{
			if(i > 0 && !(i & 7)) printf("                       ");
			printf(" %3u", bs_gets(8));
			if((i & 7) == 7) printf("\n");
		}
	}
	bs_align(8);
	bs_nextcode(0x000001, 3);
	if(bs_peek(32) == 0x000001b5)
	{
		bs_gets(32);
		printf("extension_data: %u bytes\n", bs_nextcode(0x000001, 3));
	}
	if(bs_peek(32) == 0x000001b2)
	{
		bs_gets(32);
		printf("user_data: %u bytes\n", bs_nextcode(0x000001, 3));
	}
	return NULL;
}

static const char* gop()
{
	bs_get(32);
	printf("---- GROUP OF PICTURES ----\n");
	printf("time_code: %u\n", bs_get(25));
	printf("type:");
	if(bs_gets(1) == 0b1) printf(" closed");
	if(bs_gets(1) == 0b1) printf(" broken_link");
	printf("\n");
	bs_align(8);
	bs_nextcode(0x000001, 3);
	if(bs_peek(32) == 0x000001b5)
	{
		bs_gets(32);
		printf("extension_data: %u bytes\n", bs_nextcode(0x000001, 3));
	}
	if(bs_peek(32) == 0x000001b2)
	{
		bs_gets(32);
		printf("user_data: %u bytes\n", bs_nextcode(0x000001, 3));
	}
	while(bs_peek(32) == 0x00000100) CALL(picture());
	return NULL;
}

static const char* picture()
{
	bs_get(32);
	printf("---- PICTURE ----\n");
	printf("temp_ref: %u\n", bs_get(10));
	pic_coding_type = bs_gets(3);
	printf("pic_coding_type: %d (%c)\n", pic_coding_type, pct_string[pic_coding_type]);
	printf("vbv_delay: %u\n", bs_get(16));
	if(pic_coding_type == 2 || pic_coding_type == 3)
	{
		printf("full_pel_fw_vector: %d\n", full_pel_fw_vector = bs_gets(1));
		printf("fw_f_code: %d\n", fw_f_code = bs_gets(3));
	}
	if(pic_coding_type == 3)
	{
		printf("full_pel_bw_vector: %d\n", full_pel_bw_vector = bs_gets(1));
		printf("bw_f_code: %d\n", bw_f_code = bs_gets(3));
	}
	int ei = 0;
	while(bs_gets(1) == 0b1)
	{
		if(!ei) printf("extra_info_pic:");
		printf(" %02x", bs_gets(8));
		ei = 1;
	}
	if(ei) printf("\n");
	bs_align(8);
	bs_nextcode(0x000001, 3);
	if(bs_peek(32) == 0x000001b5)
	{
		bs_gets(32);
		printf("extension_data: %u bytes\n", bs_nextcode(0x000001, 3));
	}
	if(bs_peek(32) == 0x000001b2)
	{
		bs_gets(32);
		printf("user_data: %u bytes\n", bs_nextcode(0x000001, 3));
	}
	uint32_t n;
	do
	{
		CALL(slice());
		n = bs_peek(32);
	}
	while(0x00000101 <= n && n <= 0x000001af);
	return NULL;
}


static const char* slice()
{
	uint32_t v = bs_get(32);
	printf("---- SLICE (0x%08x) ----\n", v);
	printf("quant_scale: %d\n", quant_scale = bs_gets(5));
	int ei = 0;
	while(bs_gets(1) == 0b1)
	{
		if(!ei) printf("extra_info_slice:");
		printf(" %02x", bs_gets(8));
		ei = 1;
	}
	if(ei) printf("\n");
	do
	{
		CALL(macroblock());
	}
	while(bs_peek(23) != 0);
	bs_align(8);
	bs_nextcode(0x000001, 3);
	return NULL;
}


static const char* macroblock()
{
	printf("---- MACROBLOCK ----\n");
	while(bs_peek(11) == 0b00000001111) bs_get(11);
	while(bs_peek(11) == 0b00000001000) bs_get(11);
	printf("mb_addr_inc: %d\n", mb_addr_inc = bs_vlc(vlc_table_b1));
	switch(pic_coding_type)
	{
	case 0b001:		// I
		mb_type = bs_vlc(vlc_table_b2a); break;
	case 0b010:		// P
		mb_type = bs_vlc(vlc_table_b2b); break;
	case 0b011:		// B
		mb_type = bs_vlc(vlc_table_b2c); break;
	case 0b100:		// D
		mb_type = bs_vlc(vlc_table_b2d); break;
	}
	mb_quant = (mb_type >> 4) & 1;
	mb_mo_fw = (mb_type >> 3) & 1;
	mb_mo_bw = (mb_type >> 2) & 1;
	mb_pat   = (mb_type >> 1) & 1;
	mb_intra = (mb_type >> 0) & 1;
	printf("mb_type: quant=%d, mo_fw=%d, mo_bw=%d, pat=%d, intra=%d\n",
		mb_quant, mb_mo_fw, mb_mo_bw, mb_pat, mb_intra);
	return NULL;
}

