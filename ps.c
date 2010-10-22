//================================================================================
// m2v_dec_eval - PS デコーダ
// ref: ISO13818-1
// $Id$
//================================================================================

#include <stdio.h>
#include "m2vdec.h"
#include "ps.h"

static const char* pack_header();
static const char* system_header();
static const char* pes_packet();

int g_ps_packet;


const char* decode_pack()
{
	do
	{
		CALL(pack_header());
		uint32_t n = bs_peek(32);
		while(n >= 0x000001bc && n <= 0x000001ff)
		{
			CALL(pes_packet());
			n = bs_peek(32);
		}
	}
	while(bs_peek(32) == 0x000001ba);
	return NULL;
}

static const char* pack_header()
{
	bs_align(8);
	bs_nextcode(0x000001ba, 4);
	bs_get(32);
	printf("---- PACK HEADER ----\n");
	int m2v = 0;
	if(bs_peek(2) == 0b01)
	{
		m2v = 1;
		bs_gets(2);
	}
	else if(bs_gets(4) != 0b0010)
		return "illegal bit pattern";

	printf("sys_clk_ref1: %u\n", bs_gets(3));
	if(bs_gets(1) != 0b1) return "illegal marker-bit 1";
	printf("sys_clk_ref2: %u\n", bs_get(15));
	if(bs_gets(1) != 0b1) return "illegal marker-bit 2";
	printf("sys_clk_ref3: %u\n", bs_get(15));
	if(bs_gets(1) != 0b1) return "illegal marker-bit 3";
	if(m2v) printf("sys_clk_ref_ext: %u\n", bs_get(9));
	if(bs_gets(1) != 0b1) return "illegal marker-bit 4";
	printf("prg_mux_rate: %u\n", bs_get(22));
	if(bs_gets(1) != 0b1) return "illegal marker-bit 5";
	if(m2v)
	{
		if(bs_gets(1) != 0b1) return "illegal marker-bit 6";
		bs_gets(5);
		int stuff_len = bs_gets(3);
		printf("stuff_len: %u\n", stuff_len);
		for(int i = 0; i < stuff_len; ++i)
			if(bs_gets(8) != 0xff) return "illegal stuffing byte";
	}
	if(bs_peek(32) == 0x000001bb) return system_header();
	return NULL;
}

static const char* system_header()
{
	if(bs_get(32) != 0x000001bb) return "illegal sys_hdr_start_code";
	printf("---- SYSTEM HEADER ----\n");
	uint16_t hlen = bs_get(16);
	printf("sys_hdr_len: %u bytes\n", hlen);
	if(bs_gets(1) != 0b1) return "illegal marker-bit 1 (sh)";
	printf("rate_bound: %u\n", bs_get(22));
	if(bs_gets(1) != 0b1) return "illegal marker-bit 2 (sh)";
	printf("audio_bound: %u\n", bs_gets(6));
	uint8_t flags = bs_gets(4);
	if(bs_gets(1) != 0b1) return "illegal marker-bit 3 (sh)";
	printf("flags:");
	if(flags & 0b1000) printf(" fixed");
	if(flags & 0b0100) printf(" CSPS");
	if(flags & 0b0010) printf(" sys_aud_lock");
	if(flags & 0b0001) printf(" sys_vid_lock");
	printf("\n");
	printf("video_bound: %u\n", bs_gets(5));
	printf("packet_rate_rest_flag: %u\n", bs_gets(1));
	if(bs_gets(7) != 0b1111111) return "illegal reserved bits (sh)";
	while(bs_peek(1) == 0b1)
	{
		int id = bs_gets(8);
		printf("stream_id: %u\n", id);
		if(id < 0xbc && id != 0xb8 && id != 0xb9)
			return "illegal stream id";
		if(bs_gets(2) != 0b11) return "illegal bit_pattern";
		printf("std_buf_bound_scale: %u\n", bs_gets(1));
		printf("std_buf_size_bound: %u\n", bs_get(13));
	}
	return NULL;
}

static const char* pes_packet()
{
	uint32_t id = bs_get(32);
	int len = bs_get(16);
	switch(id)
	{
	case 0x000001bf:	// private 2
	case 0x000001f0:	// ECM
	case 0x000001f1:	// EEM
	case 0x000001f2:	// DSMCC
	case 0x000001f8:	// ITU-T E
		// non-data
		for(int i = 0; i < len; ++i) bs_gets(8);
		return NULL;
	case 0x000001be:	// padding
		for(int i = 0; i < len; ++i)
		{
			uint8_t v = bs_gets(8);
			if(v == 0x0f) return NULL;	// TODO:暫定的な終了判定
			if(v != 0xff) return "illegal padding byte";
		}
		return NULL;
	}

	if(0x000001c0 <= id && id <= 0x000001df)
	{
		// audio data
		for(int i = 0; i < len; ++i)
		{
			uint8_t v = bs_gets(8);
			fwrite(&v, 1, 1, fpout_a);
		}
		return NULL;
	}

	if(0x000001e0 <= id && id <= 0x000001ef)
	{
		// video data
		for(int i = 0; i < len; ++i)
		{
			uint8_t v = bs_gets(8);
			fwrite(&v, 1, 1, fpout_v);
		}
		return NULL;
	}

	// case 0x000001bc:	// map table
	// case 0x000001bd:	// private 1
	// case 0x000001f3:	// ISO13522
	// case 0x000001f4:	// ITU-T A
	// case 0x000001f5:	// ITU-T B
	// case 0x000001f6:	// ITU-T C
	// case 0x000001f7:	// ITU-T D
	// case 0x000001ff:	// directory
	fprintf(stderr, "unknown/non-supported id: 0x%08x\n", id);
	return "packet id error";
}


