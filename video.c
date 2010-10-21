//================================================================================
// m2v_dec_eval - MPEG2 ビデオ デコーダ
// ref: ISO13818-2?
// $Id$
//================================================================================

#define _GNU_SOURCE
#define _USE_MATH_DEFINES
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "m2vdec.h"
#include "bitstream.h"
#include "video.h"
#include "vlc.h"
#include "dump.h"

static const char* sequence_header();
static const char* gop();
static const char* picture();
static const char* slice();
static const char* macroblock();
static const char* block(int bn);
static const char* dequant(int bn);
static const char* idct(int bn);

int video_wd, video_ht;
int nslice, nmb;
int max_slices, skip_slices;
int pic_coding_type;
int full_pel_fw_vector, full_pel_bw_vector;
int fw_f_code, bw_f_code;
int quant_scale;
int mb_addr_inc, mb_type;
int mb_quant, mb_mo_fw, mb_mo_bw, mb_pat, mb_intra;
int mb_quant_scale;
int mo_h_fw_code, mo_h_fw_r;
int mo_v_fw_code, mo_v_fw_r;
int mo_h_bw_code, mo_h_bw_r;
int mo_v_bw_code, mo_v_bw_r;
int dc_pred[3];		// 0:Y, 1:Cb, 2:Cr
int pat_code[6];
int qfs[64], qf[8][8], lf[8][8], sf[8][8], d[8][8];
int intra_qmat[8][8], nonintra_qmat[8][8];

static const char* const pct_string = "0IPBD567";
static const char* const blk_desc[6] = {"Y0", "Y1", "Y2", "Y3", "Cb", "Cr"};
static const int zigzag_scan[8][8] = {
	{ 0, 1, 5, 6, 14, 15, 27, 28 },
	{ 2, 4, 7, 13, 16, 26, 29, 42 },
	{ 3, 8, 12, 17, 25, 30, 41, 43 },
	{ 9, 11, 18, 24, 31, 40, 44, 53 },
	{ 10, 19, 23, 32, 39, 45, 52, 54 },
	{ 20, 22, 33, 38, 46, 51, 55, 60 },
	{ 21, 34, 37, 47, 50, 56, 59, 61 },
	{ 35, 36, 48, 49, 57, 58, 62, 63 },
};
static const int def_intra_qmat[8][8] = {
	{  8, 16, 19, 22, 26, 27, 29, 34 },
	{ 16, 16, 22, 24, 27, 29, 34, 37 },
	{ 19, 22, 26, 27, 29, 34, 34, 38 },
	{ 22, 22, 26, 27, 29, 34, 37, 40 },
	{ 22, 26, 27, 29, 32, 35, 40, 48 },
	{ 26, 27, 29, 32, 35, 40, 48, 58 },
	{ 26, 27, 29, 34, 38, 46, 56, 69 },
	{ 27, 29, 35, 38, 46, 56, 69, 83 },
};

const char* decode_video(const char* ref_dir, int slices, int skips)
{
	max_slices = slices;
	skip_slices = skips;
	nslice = 0;
	CALL(dump_init(ref_dir));
	while(bs_peek(32) == 0x000001b3)
	{
		CALL(sequence_header());
		while(bs_peek(32) == 0x000001b8)
		{
			CALL(gop());
			if(nslice == max_slices) return NULL;
		}
	}
	if(bs_get(32) != 0x000001b7) return "illegal sequence end code";
	dump_finish();
	return NULL;
}

static const char* sequence_header()
{
	bs_get(32);
	printf("---- SEQUENCE HEADER ----\n");
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
			printf(" %3d", intra_qmat[0][i] = bs_gets(8));
			if((i & 7) == 7) printf("\n");
		}
	}
	else memcpy(intra_qmat, def_intra_qmat, sizeof(intra_qmat));
	if(bs_gets(1) == 0b1)
	{
		printf("non_intra_quant_matrix:");
		for(int i = 0; i < 64; ++i)
		{
			if(i > 0 && !(i & 7)) printf("                       ");
			printf(" %3d", nonintra_qmat[0][i] = bs_gets(8));
			if((i & 7) == 7) printf("\n");
		}
	}
	else for(int i = 0; i < 64; ++i) nonintra_qmat[0][i] = 16;
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
	while(bs_peek(32) == 0x00000100)
	{
		CALL(picture());
		if(nslice == max_slices) return NULL;
	}
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
	// nslice = 0;
	do
	{
		CALL(slice());
		if(nslice == max_slices) return NULL;
		n = bs_peek(32);
	}
	while(0x00000101 <= n && n <= 0x000001af);
	return NULL;
}


static const char* slice()
{
	uint32_t v = bs_get(32);
	if(nslice == skip_slices) dump_start();
	printf("---- SLICE (0x%08x, S:%04d) ----\n", v, nslice);
	printf("quant_scale: %d\n", quant_scale = bs_gets(5));
	dump(dump_slice, NULL, "# slice %6d", nslice);
	dump(dump_slice, "quant_scale", " %2d", quant_scale);
	int ei = 0;
	while(bs_gets(1) == 0b1)
	{
		if(!ei) printf("extra_info_slice:");
		printf(" %02x", bs_gets(8));
		ei = 1;
	}
	if(ei) printf("\n");
	nmb = 0;
	dc_pred[0] = dc_pred[1] = dc_pred[2] = 128;
	do
	{
		CALL(macroblock());
	}
	while(bs_peek(23) != 0);
	bs_align(8);
	bs_nextcode(0x000001, 3);
	nslice++;
	return NULL;
}


static const char* macroblock()
{
	printf("---- MACROBLOCK (S:%05d, M:%04d) ----\n", nslice, nmb);
	while(bs_peek(11) == 0b00000001111) bs_get(11);
	while(bs_peek(11) == 0b00000001000) bs_get(11);
	printf("mb_addr_inc: %d\n", mb_addr_inc = bs_vlc(vlc_table_b1));
	dump(dump_mb, NULL, "# slice %6d, mb %4d", nslice, nmb);
	dump(dump_mb, "mb_addr_inc", " %2d", mb_addr_inc);
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
	if(!mb_intra) dc_pred[0] = dc_pred[1] = dc_pred[2] = 128;
	printf("mb_type: quant=%d, mo_fw=%d, mo_bw=%d, pat=%d, intra=%d\n",
		mb_quant, mb_mo_fw, mb_mo_bw, mb_pat, mb_intra);
	if(mb_quant) printf("mb_quant_scale: %d\n", mb_quant_scale = bs_gets(5));
	else mb_quant_scale = 0;
	dump(dump_mb, "type qua mofw mobw pat intra", " %d %d %d %d %d %d %2d",
		pic_coding_type, mb_quant, mb_mo_fw, mb_mo_bw, mb_pat, mb_intra,
		mb_quant ? mb_quant_scale : 0);
	if(mb_mo_fw)
	{
		printf("mo_h_fw_code: %d\n", mo_h_fw_code = bs_vlc(vlc_table_b4));
		if(mo_h_fw_code != 0 && fw_f_code > 1)
			printf("mo_h_fw_r: %d\n", mo_h_fw_r = bs_get(fw_f_code - 1));
		printf("mo_v_fw_code: %d\n", mo_v_fw_code = bs_vlc(vlc_table_b4));
		if(mo_v_fw_code != 0 && fw_f_code > 1)
			printf("mo_v_fw_r: %d\n", mo_v_fw_r = bs_get(fw_f_code - 1));
	}
	else mo_h_fw_code = mo_h_fw_r = mo_v_fw_code = mo_v_fw_r = 0;
	dump(dump_mb, "mo_h_fw mo_v_fw", " %3d %3d %3d %3d",
		mo_h_fw_code, mo_h_fw_r, mo_v_fw_code, mo_v_fw_r);
	if(mb_mo_bw)
	{
		printf("mo_h_bw_code: %d\n", mo_h_bw_code = bs_vlc(vlc_table_b4));
		if(mo_h_bw_code != 0 && bw_f_code > 1)
			printf("mo_h_bw_r: %d\n", mo_h_bw_r = bs_get(bw_f_code - 1));
		printf("mo_v_bw_code: %d\n", mo_v_bw_code = bs_vlc(vlc_table_b4));
		if(mo_v_bw_code != 0 && bw_f_code > 1)
			printf("mo_v_bw_r: %d\n", mo_v_bw_r = bs_get(bw_f_code - 1));
	}
	else mo_h_bw_code = mo_h_bw_r = mo_v_bw_code = mo_v_bw_r = 0;
	dump(dump_mb, "mo_h_bw mo_v_bw", " %3d %3d %3d %3d",
		mo_h_bw_code, mo_h_bw_r, mo_v_bw_code, mo_v_bw_r);
	for(int i = 0; i < 6; ++i) pat_code[i] = mb_intra;
	if(mb_pat)
	{
		int coded_blk_pat = bs_vlc(vlc_table_b3);
		for(int i = 0; i < 6; ++i)
			if(coded_blk_pat & (1<<(5-i))) pat_code[i] = 1;
	}
	printf("pat_code:");
	for(int i = 0; i < 6; ++i) printf(" %d", pat_code[i]);
	dump(dump_mb, "pat_luma pat_chroma", " %d%d%d%d %d%d",
		pat_code[0], pat_code[1], pat_code[2], pat_code[3], pat_code[4], pat_code[5]);
	printf("\n");

	for(int i = 0; i < 6; ++i) CALL(block(i));

	if(pic_coding_type == 4 && bs_gets(1) != 0b1)
		return "illegal end-of-mb (D)";

	nmb++;
	return NULL;
}


static const char* block(int bn)
{
	memset(qfs, 0, sizeof(qfs));
	printf("---- BLOCK (S:%04d, M:%04d, B:%d) ----\n", nslice, nmb, bn);
	if(pat_code[bn])
	{
		int pos = 0;
		if(mb_intra)
		{
			if(bn < 4)
			{
				int dc_size_luma = bs_vlc(vlc_table_b5a);
				if(dc_size_luma > 0) qfs[pos++] = bs_get(dc_size_luma) + dc_pred[0];
			}
			else
			{
				int dc_size_chroma = bs_vlc(vlc_table_b5b);
				if(dc_size_chroma > 0) qfs[pos++] = bs_get(dc_size_chroma) + dc_pred[bn - 3];
			}
		}
		else
		{
			int v = bs_vlc(vlc_table_dct_f);
			int run = v / 1000;
			int level = (v % 1000) - 500;
			if(run == 999)
			{
				run = bs_gets(6);
				level = bs_gets(8);
				if(level >= 128) level -= 256;
			}
			if(level == 0) level = bs_gets(8);
			if(level == -128) level = ((int)bs_gets(8)) - 256;
			// printf("dct_coef_first: run=%d, level=%d\n", run, level);
			pos += run;
			qfs[pos++] = level;
		}
		if(pic_coding_type != 4)
		{
			while(bs_peek(2) != 0b10)
			{
				int v = bs_vlc(vlc_table_dct_n);
				int run = v / 1000;
				int level = (v % 1000) - 500;
				if(run == 999)
				{
					run = bs_gets(6);
					level = bs_gets(8);
					if(level >= 128) level -= 256;
				}
				if(level == 0) level = bs_gets(8);
				if(level == -128) level = ((int)bs_gets(8)) - 256;
				// printf("dct_coef_next: run=%d, level=%d\n", run2, level2);
				pos += run;
				if(pos >= 64) return "invalid QFS position!";
				qfs[pos++] = level;
			}
			bs_gets(2);
		}
	}

	// zigzag de-scan
	for(int u = 0; u < 8; ++u) for(int v = 0; v < 8; ++v)
		qf[v][u] = qfs[zigzag_scan[v][u]];

	// dump (before dequant)
	dump(dump_block, NULL, "# slice %6d, mb %4d, block %d (%s)",
		nslice, nmb, bn, bn > 4 ? "chroma" : "luma");
	for(int v = 0; v < 8; ++v)
	{
		char sz[3];
		sz[0] = 'v';
		sz[1] = v + '0';
		sz[2] = 0;
		dump(dump_block, sz, " %4d %4d %4d %4d %4d %4d %4d %4d ",
			qf[v][0], qf[v][1], qf[v][2], qf[v][3],
			qf[v][4], qf[v][5], qf[v][6], qf[v][7]);
	}

	// dequant
	CALL(dequant(bn));

	// idct
	if(pat_code[bn]) CALL(idct(bn));

	return NULL;
}


static int sgn(int v)
{
	if(v < 0) return -1;
	if(v > 0) return 1;
	return 0;
}


static const char* dequant(int bn)
{
	int qs = quant_scale + mb_quant_scale;
	if(mb_intra)
	{
		for(int v = 0; v < 8; ++v) for(int u = 0; u < 8; ++u)
		{
			if(!v && !u) continue;	// (0,0)は除く
			int x = 2 * qf[v][u] * qs * intra_qmat[v][u] / 16;
			if(!(x & 1)) x -= sgn(x);
			if(x > 2047) x = 2047;
			if(x < -2048) x = -2048;
			lf[v][u] = x;
		}
	}
	else
	{
		for(int v = 0; v < 8; ++v) for(int u = 0; u < 8; ++u)
		{
			int x = (2 * qf[v][u] + sgn(qf[v][u])) * qs * nonintra_qmat[v][u] / 16;
			if(!(x & 1)) x -= sgn(x);
			if(x > 2047) x = 2047;
			if(x < -2048) x = -2048;
			lf[v][u] = x;
		}
	}

	dump(dump_dequant, NULL, "# slice %6d, mb %4d, block %d (%s)",
		nslice, nmb, bn, blk_desc[bn]);
	for(int v = 0; v < 8; ++v)
		dump(dump_dequant, NULL, " %5d %5d %5d %5d %5d %5d %5d %5d",
			lf[v][0], lf[v][1], lf[v][2], lf[v][3],
			lf[v][4], lf[v][5], lf[v][6], lf[v][7]);

	return NULL;
}


const char* idct(int bn)
{
	// x_{i,j}= 2/N Σ_{k=0}^{N-1}Σ_{l=0}^{N-1}C(k)C(l)x_{k,l}cos(πk(2i+1)/2N)cos(πl(2j+1)/2N)
	// N=8
	/*
	for(int j = 0; j < 8; ++j) for(int i = 0; i < 8; ++i)
	{
		double s = 0.0;
		for(int k = 0; k < 8; ++k) for(int l = 0; l < 8; ++l)
		{
			double d = lf[l][k] * cos(M_PI * k * (2 * i + 1) / 16) * cos(M_PI * l * (2 * j + 1) / 16);
			if(k == 0) d *= M_SQRT1_2;
			if(l == 0) d *= M_SQRT1_2;
			s += d;
		}
		sf[j][i] = (int)s;
	}
	*/
	extern void simple_idct(int L[8][8], int S[8][8]);
	simple_idct(lf, sf);

	dump(dump_idct, NULL, "# slice %6d, mb %4d, block %d (%s)",
		nslice, nmb, bn, blk_desc[bn]);
	for(int y = 0; y < 8; ++y)
		dump(dump_idct, NULL, " %5d %5d %5d %5d %5d %5d %5d %5d",
			sf[y][0], sf[y][1], sf[y][2], sf[y][3],
			sf[y][4], sf[y][5], sf[y][6], sf[y][7]);
	return NULL;
}

