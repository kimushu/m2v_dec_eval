//================================================================================
// m2v_dec_eval - PS デコーダ
// ref: ISO13818-1
// $Id: ps.c 76 2010-11-28 12:41:01Z aberi $
//================================================================================

#include <stdio.h>
#include <string.h>
#include "bitreader.h"
#include "ps.h"

static const char* pack_header(bitstream* bs);
static const char* system_header(bitstream* bs);
static const char* pes_packet(bitstream* bs);

#define CALL(x)		({ const char* e = x; if(e) { printf("%s", e); return 0; } })

FILE* fpout_a;
FILE* fpout_v;

int decode_pack(const char* file)
{
	bitstream* bs = bs_open(file);
	if(!bs) return 0;

	char fn[256];
	strcpy(fn, file);
	strcat(fn, ".vs");
	if(!(fpout_v = fopen(fn, "wb")))
	{
		printf("Error: Cannot open '%s' for writing!\n", fn);
		return 0;
	}

	strcpy(fn, file);
	strcat(fn, ".as");
	if(!(fpout_a = fopen(fn, "wb")))
	{
		fclose(fpout_v);
		printf("Error: Cannot open '%s' for writing!\n", fn);
		return 0;
	}

	do
	{
		CALL(pack_header(bs));
		uint32_t n = bs_peek(bs, 32);
		while(n >= 0x000001bc && n <= 0x000001ff)
		{
			CALL(pes_packet(bs));
			n = bs_peek(bs, 32);
		}
	}
	while(bs_peek(bs, 32) == 0x000001ba);
	printf("---- END OF STREAM ----\n");
	fclose(fpout_v);
	fclose(fpout_a);
	bs_close(bs);
	return 1;
}

static const char* pack_header(bitstream* bs)
{
	bs_align(bs, 8);
	while(bs_peek(bs, 32) != 0x000001ba) bs_get(bs, 8, NULL);
	bs_get(bs, 32, NULL);
	printf("---- PACK HEADER ----\n");
	int m2v = 0;
	if(bs_peek(bs, 2) == 0b01)
	{
		m2v = 1;
		bs_get(bs, 2, NULL);
	}
	else if(bs_get(bs, 4, NULL) != 0b0010)
		return "illegal bit pattern";

	printf("sys_clk_ref1: %u\n", bs_get(bs, 3, NULL));
	if(bs_get(bs, 1, NULL) != 0b1) return "illegal marker-bit 1";
	printf("sys_clk_ref2: %u\n", bs_get(bs, 15, NULL));
	if(bs_get(bs, 1, NULL) != 0b1) return "illegal marker-bit 2";
	printf("sys_clk_ref3: %u\n", bs_get(bs, 15, NULL));
	if(bs_get(bs, 1, NULL) != 0b1) return "illegal marker-bit 3";
	if(m2v) printf("sys_clk_ref_ext: %u\n", bs_get(bs, 9, NULL));
	if(bs_get(bs, 1, NULL) != 0b1) return "illegal marker-bit 4";
	printf("prg_mux_rate: %u\n", bs_get(bs, 22, NULL));
	if(bs_get(bs, 1, NULL) != 0b1) return "illegal marker-bit 5";
	if(m2v)
	{
		if(bs_get(bs, 1, NULL) != 0b1) return "illegal marker-bit 6";
		bs_get(bs, 5, NULL);
		int stuff_len = bs_get(bs, 3, NULL);
		printf("stuff_len: %u\n", stuff_len);
		for(int i = 0; i < stuff_len; ++i)
			if(bs_get(bs, 8, NULL) != 0xff) return "illegal stuffing byte";
	}
	if(bs_peek(bs, 32) == 0x000001bb) return system_header(bs);
	return NULL;
}

static const char* system_header(bitstream* bs)
{
	if(bs_get(bs, 32, NULL) != 0x000001bb) return "illegal sys_hdr_start_code";
	printf("---- SYSTEM HEADER ----\n");
	uint16_t hlen = bs_get(bs, 16, NULL);
	printf("sys_hdr_len: %u bytes\n", hlen);
	if(bs_get(bs, 1, NULL) != 0b1) return "illegal marker-bit 1 (sh)";
	printf("rate_bound: %u\n", bs_get(bs, 22, NULL));
	if(bs_get(bs, 1, NULL) != 0b1) return "illegal marker-bit 2 (sh)";
	printf("audio_bound: %u\n", bs_get(bs, 6, NULL));
	uint8_t flags = bs_get(bs, 4, NULL);
	if(bs_get(bs, 1, NULL) != 0b1) return "illegal marker-bit 3 (sh)";
	printf("flags:");
	if(flags & 0b1000) printf(" fixed");
	if(flags & 0b0100) printf(" CSPS");
	if(flags & 0b0010) printf(" sys_aud_lock");
	if(flags & 0b0001) printf(" sys_vid_lock");
	printf("\n");
	printf("video_bound: %u\n", bs_get(bs, 5, NULL));
	printf("packet_rate_rest_flag: %u\n", bs_get(bs, 1, NULL));
	if(bs_get(bs, 7, NULL) != 0b1111111) return "illegal reserved bits (sh)";
	while(bs_peek(bs, 1) == 0b1)
	{
		int id = bs_get(bs, 8, NULL);
		printf("stream_id: %u\n", id);
		if(id < 0xbc && id != 0xb8 && id != 0xb9)
			return "illegal stream id";
		if(bs_get(bs, 2, NULL) != 0b11) return "illegal bit_pattern";
		printf("std_buf_bound_scale: %u\n", bs_get(bs, 1, NULL));
		printf("std_buf_size_bound: %u\n", bs_get(bs, 13, NULL));
	}
	return NULL;
}

static const char* pes_packet(bitstream* bs)
{
	uint32_t id = bs_get(bs, 32, NULL);
	int len = bs_get(bs, 16, NULL);
	switch(id)
	{
	case 0x000001bf:	// private 2
	case 0x000001f0:	// ECM
	case 0x000001f1:	// EEM
	case 0x000001f2:	// DSMCC
	case 0x000001f8:	// ITU-T E
		// non-data
		return "non-supported packet type";
		for(int i = 0; i < len; ++i) bs_get(bs, 8, NULL);
		return NULL;
	case 0x000001be:	// padding
		for(int i = 0; i < len; ++i)
		{
			uint8_t v = bs_get(bs, 8, NULL);
			if(v == 0x0f)
			{
				printf("---- END OF STREAM (ff) ----\n");
				return NULL;	// TODO:暫定的な終了判定
			}
			if(v != 0xff) return "illegal padding byte";
		}
		return NULL;
	}

	// skip stuffing bytes
	while(bs_peek(bs, 1))
	{
		bs_get(bs, 8, NULL);
		len -= 1;
	}

	if(bs_peek(bs, 2) == 0b01)
	{
		bs_get(bs, 2, NULL);
		printf("std_buf_scale: %u\n", bs_get(bs, 1, NULL));
		printf("std_buf_size: %u\n", bs_get(bs, 13, NULL));
		len -= 2;
	}

	uint32_t n = bs_peek(bs, 4);
	if(n == 0b0010 || n == 0b0011)
	{
		bs_get(bs, 4, NULL);
		printf("pres_time_stamp[2]: %u\n", bs_get(bs, 3, NULL));
		if(bs_get(bs, 1, NULL) == 0b0) return "illegal marker-bit (pes pts 1)";
		printf("pres_time_stamp[1]: %u\n", bs_get(bs, 15, NULL));
		if(bs_get(bs, 1, NULL) == 0b0) return "illegal marker-bit (pes pts 2)";
		printf("pres_time_stamp[0]: %u\n", bs_get(bs, 15, NULL));
		if(bs_get(bs, 1, NULL) == 0b0) return "illegal marker-bit (pes pts 3)";
		len -= 5;
	}
	if(n == 0b0011)
	{
		if(bs_get(bs, 4, NULL) != 0b0001) return "illegal id bits (pes)";
		printf("dec_time_stamp[2]: %u\n", bs_get(bs, 3, NULL));
		if(bs_get(bs, 1, NULL) == 0b0) return "illegal marker-bit (pes dts 1)";
		printf("dec_time_stamp[1]: %u\n", bs_get(bs, 15, NULL));
		if(bs_get(bs, 1, NULL) == 0b0) return "illegal marker-bit (pes dts 2)";
		printf("dec_time_stamp[0]: %u\n", bs_get(bs, 15, NULL));
		if(bs_get(bs, 1, NULL) == 0b0) return "illegal marker-bit (pes dts 3)";
		len -= 5;
	}
	else if(n != 0b0010)
	{
		if(bs_get(bs, 8, NULL) != 0b00001111) return "illegal id byte";
		len -= 1;
	}

	if(0x000001c0 <= id && id <= 0x000001df)
	{
		// audio data
		for(int i = 0; i < len; ++i)
		{
			uint8_t v = bs_get(bs, 8, NULL);
			fwrite(&v, 1, 1, fpout_a);
		}
		return NULL;
	}

	if(0x000001e0 <= id && id <= 0x000001ef)
	{
		// video data
		printf("VIDEO PACKET (0x%08x): %d bytes\n", id, len);
		for(int i = 0; i < len; ++i)
		{
			uint8_t v = bs_get(bs, 8, NULL);
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


