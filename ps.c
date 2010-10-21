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
static const char* packet_header(uint16_t* len);
static const char* padding_ps_packet();
static const char* audio_packet();
static const char* video_packet();
// static const char* pes_packet();

int g_ps_packet;


const char* decode_pack()
{
	while(1)	// TODO:終了条件
	{
		uint32_t n = bs_peek(32);
		if(n == 0x000001ba) CALL(pack_header());
		else if(n == 0x000001be) CALL(padding_ps_packet());
		else if(0x000001c0 <= n && n <= 0x000001df)
			CALL(audio_packet());
		else if(0x000001e0 <= n && n <= 0x000001ef)
			CALL(video_packet());
		else if(n == 0xffffffff)	// TODO:仮の終了条件
			break;
		else
		{
			fprintf(stderr, "(0x%08x @ P%06u)", n, g_ps_packet);
			return "unknown packet id";
		}
	}
	return NULL;
}

static const char* pack_header()
{
	if(bs_get(32) != 0x000001ba) return "illegal pack_start_code";
	printf("---- PACK HEADER ----\n");
	if(bs_gets(4) != 0b0010) return "illegal filler";
	printf("sys_clk_ref1: %u\n", bs_gets(3));
	if(bs_gets(1) != 0b1) return "illegal marker-bit 1";
	printf("sys_clk_ref2: %u\n", bs_get(15));
	if(bs_gets(1) != 0b1) return "illegal marker-bit 2";
	printf("sys_clk_ref3: %u\n", bs_get(15));
	if(bs_gets(1) != 0b1) return "illegal marker-bit 3";
	if(bs_gets(1) != 0b1) return "illegal marker-bit 4";
	printf("mux_rate: %u\n", bs_get(22));
	if(bs_gets(1) != 0b1) return "illegal marker-bit 5";
	uint32_t n = bs_peek(32);
	if(n == 0x000001bb) CALL(system_header());
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
	if(bs_gets(8) != 0b11111111) return "illegal reserved bits (sh)";
	while(bs_peek(1) == 0b1)
	{
		printf("stream_id: %u\n", bs_gets(8));
		bs_gets(2);
		printf("std_buf_bound_scale: %u\n", bs_gets(1));
		printf("std_buf_size_bound: %u\n", bs_gets(13));
	}
	return NULL;
}

static const char* packet_header(uint16_t* len)
{
	*len = 0;
	printf("---- PACKET HEADER ----\n");
	while(bs_peek(8) == 0b11111111)
	{
		bs_gets(8);
		(*len) += 1;
	}

	if(bs_peek(2) == 0b01)
	{
		bs_gets(2);
		printf("std_buf_scale: %u\n", bs_gets(1));
		printf("std_buf_size: %u\n", bs_get(13));
		(*len) += 2;
	}
	uint32_t n = bs_peek(4);
	if(n == 0b0010 || n == 0b0011)
	{
		bs_gets(4);
		printf("pres_time_stamp1: %u\n", bs_gets(3));
		if(bs_gets(1) != 0b1) return "illegal marker-bit 1 (ph)";
		printf("pres_time_stamp2: %u\n", bs_get(15));
		if(bs_gets(1) != 0b1) return "illegal marker-bit 2 (ph)";
		printf("pres_time_stamp3: %u\n", bs_get(15));
		if(bs_gets(1) != 0b1) return "illegal marker-bit 3 (ph)";
		(*len) += 5;
	}
	if(n == 0b0011)
	{
		if(bs_gets(4) != 0b0001) return "illegal filler (ph)";
		printf("dec_time_stamp1: %u\n", bs_gets(3));
		if(bs_gets(1) != 0b1) return "illegal marker-bit 4 (ph)";
		printf("dec_time_stamp2: %u\n", bs_get(15));
		if(bs_gets(1) != 0b1) return "illegal marker-bit 5 (ph)";
		printf("dec_time_stamp3: %u\n", bs_get(15));
		if(bs_gets(1) != 0b1) return "illegal marker-bit 6 (ph)";
		(*len) += 5;
	}
	else if(n != 0b0010)
	{
		bs_gets(8);
		(*len) += 1;
	}
	return NULL;
}

static const char* padding_ps_packet()
{
	bs_get(32);
	printf("---- [%06u] PADDING PACKET ----\n", g_ps_packet++);
	uint16_t plen = bs_get(16);
	uint16_t hlen;
	CALL(packet_header(&hlen));
	plen -= hlen;
	for(uint16_t i = 0; i < plen; ++i) bs_gets(8);
	printf("data bytes: %u\n", plen);
	return NULL;
}

static const char* audio_packet()
{
	uint32_t v = bs_get(32);
	printf("---- [%06u] AUDIO PACKET (0x%08x) ----\n", g_ps_packet++, v);
	uint16_t plen = bs_get(16);
	uint16_t hlen;
	CALL(packet_header(&hlen));
	plen -= hlen;
	for(uint16_t i = 0; i < plen; ++i)
	{
		// if(!(i & 15)) printf("\n+%04xh", i);
		// printf(" %02x", bs_gets(8));
		uint8_t b = bs_gets(8);
		fwrite(&b, 1, 1, fpout_a);
	}
	// printf("\n");
	printf("data bytes: %u\n", plen);
	return NULL;
}

static const char* video_packet()
{
	uint32_t v = bs_get(32);
	printf("---- [%06u] VIDEO PACKET (0x%08x) ----\n", g_ps_packet++, v);
	uint16_t plen = bs_get(16);
	uint16_t hlen;
	CALL(packet_header(&hlen));
	plen -= hlen;
	for(uint16_t i = 0; i < plen; ++i)
	{
		// if(!(i & 15)) printf("\n+%04xh", i);
		// printf(" %02x", bs_gets(8));
		uint8_t b = bs_gets(8);
		fwrite(&b, 1, 1, fpout_v);
	}
	// printf("\n");
	printf("data bytes: %u\n", plen);
	return NULL;
}

// static const char* pes_packet()
// {
// 	return NULL;
// }

