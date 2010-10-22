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
static const char* extension_and_user_data(int i);
static const char* extension_data(int i);
static const char* user_data();
static const char* sequence_extension();
//static const char* sequence_display_extension();
//static const char* sequence_scalable_extension();
static const char* group_of_pictures_header();
static const char* picture_header();
static const char* picture_coding_extension();
static const char* picture_data();
static const char* slice();
static const char* macroblock();
static const char* macroblock_modes();
static const char* motion_vectors(int s);
static const char* motion_vector(int r, int s);
static const char* coded_block_pattern();
static const char* block(int b);
static const char* dequant(int b);
static const char* idct(int b);

// sequence_header
int horz_size, vert_size, aspect_ratio, frame_rate_code,
	frame_rate_n, frame_rate_d,
	bitrate_value, vbv_buf_size, const_param_flag;
int intra_qmat[8][8], nonintra_qmat[8][8];

// extension data
int seq_scalable;

// seq extension
int prof_and_level, prog_seq, chroma_fmt, low_delay, block_count;

// gop header
int time_code, closed_gop, broken_link;

// picture header
int temp_ref, pic_coding_type, vbv_delay,
	full_pel_fw_vector, fw_f_code, full_pel_bw_vector, bw_f_code;

// picture coding extension
int f_code[2][2], intra_dc_precision, picture_struct, top_field_first,
	frame_pred_frame_dct, conceal_mv, q_scale_type, intra_vlc_fmt,
	alt_scan, rep_first_field, chroma_420_type, prog_frame,
	v_axis, field_seq, sub_carrier, burst_amp, sub_car_phase;

// slice
int nslice, max_slices, skip_slices;
int q_scale_code, intra_slice;

// macroblock
int nmb;
int mb_addr_inc, mb_quant, mb_q_scale_code, mb_mo_fw, mb_mo_bw,
	mb_pattern, mb_intra;
int field_mo_type, frame_mo_type, dct_type, mv_count, mv_format, dmv;

// motion vectors
int mo_vert_field_select[2][2], mo_code[2][2][2], mo_residual[2][2][2],
	dmvector[2];

// coded block pattern
int cbp420, cbp1, cbp2, pattern_code[12];


static const char* const PCT_STRING = "0IPB4567";	// '4'=='D' if ISO11172-2
static const char* const CHROMA_FMT_NAME[] = {"reserved", "4:2:0", "4:2:2", "4:4:4"};
static const int BLK_COUNT[4] = {0, 6, 8, 12};
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

/*
	picture start code   00
	slice start code     01-af
	user data start code b2
	seq header code      b3
	seq error code       b4
	ext start code       b5
	seq end code         b7
	group start code     b8
	system start codes   b9-ff
*/

#define PICTURE_START_CODE	0x00000100
#define UDATA_START_CODE	0x000001b2
#define SEQ_HEADER_CODE		0x000001b3
#define SEQ_ERROR_CODE		0x000001b4
#define EXT_START_CODE		0x000001b5
#define SEQ_END_CODE		0x000001b7
#define GROUP_START_CODE	0x000001b8

#define SEQ_EXT_ID			0b0001
#define PIC_CODE_EXT_ID		0b1000

#define PIC_STRUCT_TOPF		1
#define PIC_STRUCT_BTMF		2
#define PIC_STRUCT_FRAME	3

const char* decode_video(const char* ref_dir, int slices, int skips)
{
	max_slices = slices;
	skip_slices = skips;
	nslice = 0;
	CALL(dump_init(ref_dir));

	// ISO-13818-2-DRAFT p.42
	bs_nextcode(0x000001, 3);

	uint32_t n;
	do
	{
		CALL(sequence_header());
		if(bs_peek(32) != EXT_START_CODE)
			return "stream is mpeg1video! (not supported)";	// ISO11172-2

		CALL(sequence_extension());
		CALL(extension_and_user_data(0));
		do
		{
			if(bs_peek(32) == GROUP_START_CODE)
			{
				CALL(group_of_pictures_header());
				CALL(extension_and_user_data(1));
			}
			CALL(picture_header());
			CALL(picture_coding_extension());
			CALL(extension_and_user_data(2));
			CALL(picture_data());
			n = bs_peek(32);
		}
		while(n == PICTURE_START_CODE || n == GROUP_START_CODE);
	}
	while(n != SEQ_END_CODE);
	bs_get(32);
	dump_finish();
	return NULL;
}

static const char* sequence_header()
{
	uint32_t n = bs_get(32);
	printf("---- SEQUENCE HEADER (0x%08x) ----\n", n);
	seq_scalable = 0;	// init
	if(n != SEQ_HEADER_CODE) return "illegal SEQ_HEADER_CODE";
	horz_size = bs_get(12);
	vert_size = bs_get(12);
	printf("size: %d x %d\n", horz_size, vert_size);
	printf("aspect_ratio: %u\n", aspect_ratio = bs_gets(4));
	printf("frame_rate_code: %u\n", frame_rate_code = bs_gets(4));
	frame_rate_d = 1;
	switch(frame_rate_code)
	{
	case 1: frame_rate_n = 24000; frame_rate_d = 1001; break;
	case 2: frame_rate_n = 24; break;
	case 3: frame_rate_n = 25; break;
	case 4: frame_rate_n = 30000; frame_rate_d = 1001; break;
	case 5: frame_rate_n = 30; break;
	case 6: frame_rate_n = 50; break;
	case 7: frame_rate_n = 60000; frame_rate_d = 1001; break;
	case 8: frame_rate_n = 60; break;
	default: return "illegal frame_rate_code!";
	}
	printf("frame_rate: %lf (%u/%u)\n", (double)frame_rate_n / frame_rate_d,
		frame_rate_n, frame_rate_d);
	printf("bitrate: %u\n", bitrate_value = bs_get(18));
	if(bs_gets(1) != 0b1) return "illegal marker-bit (seqh)";
	printf("vbv_buf_size: %u\n", vbv_buf_size = bs_get(10));
	printf("const_param_flag: %u\n", const_param_flag = bs_gets(1));
	if(bs_gets(1) == 0b1)
	{
		printf("intra_quant_matrix:\n");
		for(int i = 0; i < 64; ++i)
			printf(" %3d%s", intra_qmat[0][i] = bs_gets(8),
				(i & 7) == 7 ? "\n" : "");
	}
	else memcpy(intra_qmat, def_intra_qmat, sizeof(intra_qmat));
	if(bs_gets(1) == 0b1)
	{
		printf("non_intra_quant_matrix:");
		for(int i = 0; i < 64; ++i)
			printf(" %3d%s", nonintra_qmat[0][i] = bs_gets(8),
				(i & 7) == 7 ? "\n" : "");
	}
	else for(int i = 0; i < 64; ++i) nonintra_qmat[0][i] = 16;
	bs_align(8);
	bs_nextcode(0x000001, 3);
	return NULL;
}

static const char* extension_and_user_data(int i)
{
	uint32_t n = bs_peek(32);
	while(n == EXT_START_CODE || n == UDATA_START_CODE)
	{
		if(n == EXT_START_CODE && i != 1)
			CALL(extension_data(i));
		else if(n == UDATA_START_CODE)
			CALL(user_data());
	}
	return NULL;
}

static const char* extension_data(int i)
{
	while(bs_peek(32) == EXT_START_CODE)
	{
		bs_get(32);
		if(i == 0)
		{
			return "not implemented (display/scalable extension)";
		}
		else if(i == 2)
		{
			return "not implemented (quant/cr/picdisp/picspa/pictemp extension)";
		}
	}
	return NULL;
}

static const char* user_data()
{
	uint32_t n = bs_get(32);
	printf("---- USER DATA (0x%08x) ----\n", n);
	if(n != UDATA_START_CODE) return "illegal UDATA_START_CODE";
	int i = bs_nextcode(0x000001, 3);
	printf("user_data: %d bytes\n", i);
	return NULL;
}

static const char* sequence_extension()
{
	uint32_t n = bs_get(32);
	printf("---- SEQUENCE EXTENSION (0x%08x) ----\n", n);
	if(n != EXT_START_CODE) return "illegal EXT_START_CODE";
	if(bs_gets(4) != SEQ_EXT_ID) return "illegal extension id (SEQ_EXT_ID)";
	printf("profile_and_level: %u\n", prof_and_level = bs_gets(8));
	printf("prog_seq: %u\n", prog_seq = bs_gets(1));
	chroma_fmt = bs_gets(2);
	printf("chroma_format: %u (%s)\n", chroma_fmt, CHROMA_FMT_NAME[chroma_fmt]);
	block_count = BLK_COUNT[chroma_fmt];
	printf("horz_size w/ext: %u\n", horz_size += (bs_gets(2) << 12));
	printf("vert_size w/ext: %u\n", vert_size += (bs_gets(2) << 12));
	printf("bitrate w/ext: %u\n", bitrate_value += (bs_get(12) << 18));
	if(bs_gets(1) != 0b1) return "illegal marker-bit (seq-ext)";
	printf("vbv_buf_size w/ext: %u\n", vbv_buf_size += (bs_gets(8) << 10));
	printf("low_delay: %u\n", low_delay = bs_gets(1));
	frame_rate_n *= (bs_gets(2) + 1);
	frame_rate_d *= (bs_gets(5) + 1);
	printf("frame_rate w/ext: %lf (%u/%u)\n", (double)frame_rate_n / frame_rate_d,
		frame_rate_n, frame_rate_d);
	bs_align(8);
	bs_nextcode(0x000001, 3);
	return NULL;
}

/*
static const char* sequence_display_extension()
{
	if(bs_gets(4) != 0b0010) return "illegal extension id (!=0b0010)";
	printf("---- SEQUENCE DISPLAY EXTENSION ----\n");
	printf("video_fmt: %u\n", bs_gets(3));
	int cd = bs_gets(1);
	printf("color_desc: %u\n", cd);
	if(cd)
	{
		printf("color_primaries: %u\n", bs_gets(8));
		printf("trans_char: %u\n", bs_gets(8));
		printf("mat_coef: %u\n", bs_gets(8));
	}
	printf("disp_horz_size: %u\n", bs_get(14));
	if(bs_gets(1) != 0b1) return "illegal marker-bit (sde)";
	printf("disp_vert_size: %u\n", bs_get(14));
	bs_align(8);
	bs_nextcode(0x000001, 3);
	return NULL;
}

static const char* sequence_scalable_extension()
{
	if(bs_gets(4) != 0b0101) return "illegal extension id (!=0b0101)";
	printf("---- SEQUENCE SCALABLE EXTENSION ----\n");
	printf("scalable_mode: %u\n", bs_gets(2));
	printf("layer_id: %u\n", bs_gets(4));
	printf("skipped...\n");	// TODO
	bs_align(8);
	bs_nextcode(0x000001, 3);
	return NULL;
}
*/

static const char* group_of_pictures_header()
{
	uint32_t n = bs_get(32);
	printf("---- GROUP OF PICTURE HEADER (0x%08x) ----\n", n);
	if(n != GROUP_START_CODE) return "illegal GROUP_START_CODE";
	printf("time_code: %u\n", time_code = bs_get(25));
	printf("closed_gop: %u\n", closed_gop = bs_gets(1));
	printf("broken_link: %u\n", broken_link = bs_gets(1));
	bs_align(8);
	bs_nextcode(0x000001, 3);
	return NULL;
}

static const char* picture_header()
{
	uint32_t n = bs_get(32);
	printf("---- PICTURE HEADER (0x%08x) ----\n", n);
	if(n != PICTURE_START_CODE) return "illegal PICTURE_START_CODE";
	printf("temp_ref: %u\n", temp_ref = bs_get(10));
	pic_coding_type = bs_gets(3);
	printf("pic_coding_type: %d (%c)\n", pic_coding_type, PCT_STRING[pic_coding_type]);
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
	return NULL;
}

static const char* picture_coding_extension()
{
	uint32_t n = bs_get(32);
	printf("---- PICTURE CODING EXTENSION (0x%08x:%x) ----\n", n, bs_peek(4));
	if(n != EXT_START_CODE) return "illegal EXT_START_CODE";
	if(bs_gets(4) != PIC_CODE_EXT_ID) return "illegal ext id (PIC_CODE_EXT_ID)";
	f_code[0][0] = bs_gets(4);
	f_code[0][1] = bs_gets(4);
	f_code[1][0] = bs_gets(4);
	f_code[1][1] = bs_gets(4);
	printf("f_code: {{%u, %u}, {%u, %u}}\n",
		f_code[0][0], f_code[0][1], f_code[1][0], f_code[1][1]);
	printf("intra_dc_precision: %u\n", intra_dc_precision = bs_gets(2));
	printf("picture_struct: %u\n", picture_struct = bs_gets(2));
	printf("top_field_first: %u\n", top_field_first = bs_gets(1));
	printf("frame_pred_frame_dct: %u\n", frame_pred_frame_dct = bs_gets(1));
	printf("conceal_mv: %u\n", conceal_mv = bs_gets(1));
	printf("q_scale_type: %u\n", q_scale_type = bs_gets(1));
	printf("intra_vlc_fmt: %u\n", intra_vlc_fmt = bs_gets(1));
	printf("alt_scan: %u\n", alt_scan = bs_gets(1));
	printf("rep_first_field: %u\n", rep_first_field = bs_gets(1));
	printf("chroma_420_type: %u\n", chroma_420_type = bs_gets(1));
	printf("prog_frame: %u\n", prog_frame = bs_gets(1));
	if(bs_gets(1) == 0b1)
	{
		printf("v_axis: %u\n", v_axis = bs_gets(1));
		printf("field_seq: %u\n", field_seq = bs_gets(3));
		printf("sub_carrier: %u\n", sub_carrier = bs_gets(1));
		printf("burst_amp: %u\n", burst_amp = bs_gets(7));
		printf("sub_car_phase: %u\n", sub_car_phase = bs_gets(8));
	}
	bs_align(8);
	bs_nextcode(0x000001, 3);
	return NULL;
}

static const char* picture_data()
{
	uint32_t n;
	do
	{
		CALL(slice());
		if(nslice == max_slices) return NULL;
		n = bs_peek(32);
	}
	while(n == PICTURE_START_CODE);
	return NULL;
}

static const char* slice()
{
	uint32_t n = bs_get(32);
	if(nslice == skip_slices) dump_start();
	printf("---- SLICE (0x%08x, S:%04d) ----\n", n, nslice);
	if(vert_size > 2800)
		return "video too large!"; // slice_vertical_position_extension
	if(seq_scalable)
		return "scalable is not supported!";
	printf("q_scale_code: %u\n", q_scale_code = bs_gets(5));
	if(bs_gets(1) == 0b1)
	{
		printf("intra_slice: %u\n", intra_slice = bs_gets(1));
		bs_gets(7);
		int ei = 0;
		while(bs_gets(1) == 0b1)
		{
			if(!ei) printf("extra_info_slice:");
			printf(" %02x", bs_gets(8));
			ei = 1;
		}
		if(ei) printf("\n");
	}
	else intra_slice = 0;
	nmb = 0;
	// dc_pred[0] = dc_pred[1] = dc_pred[2] = 128;
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
	while(bs_peek(11) == 0b00000001000) bs_get(11);	// escape
	printf("mb_addr_inc: %d\n", mb_addr_inc = bs_vlc(vlc_table_b1));
	// dump(dump_mb, NULL, "# slice %6d, mb %4d", nslice, nmb);
	// dump(dump_mb, "mb_addr_inc", " %2d", mb_addr_inc);
	CALL(macroblock_modes());
	if(mb_quant) printf("mb_q_scale_code: %d\n", mb_q_scale_code = bs_gets(5));
	else mb_q_scale_code = 0;
	if(mb_mo_fw || (mb_intra && conceal_mv)) CALL(motion_vectors(0));
	if(mb_mo_bw) CALL(motion_vectors(1));
	if(mb_intra && conceal_mv && bs_gets(1) != 0b1)
		return "illegal marker-bit (mb)";
	if(mb_pattern) CALL(coded_block_pattern());
	for(int b = 0; b < block_count; ++b) CALL(block(b));
	return NULL;
}

static const char* macroblock_modes()
{
	if(seq_scalable) return "scalable is not supported!"; // TODO: scalable
	int mb_type;
	switch(pic_coding_type)
	{
	case 1:		// I
		mb_type = bs_vlc(vlc_table_b2); break;
	case 2:		// P
		mb_type = bs_vlc(vlc_table_b3); break;
	case 3:		// B
		mb_type = bs_vlc(vlc_table_b4); break;
	default:
		return "unknown pic_coding_type";
	}
	mb_quant   = (mb_type >> 7) & 1;
	mb_mo_fw   = (mb_type >> 6) & 1;
	mb_mo_bw   = (mb_type >> 5) & 1;
	mb_pattern = (mb_type >> 4) & 1;
	mb_intra   = (mb_type >> 3) & 1;
	// if(!mb_intra) dc_pred[0] = dc_pred[1] = dc_pred[2] = 128;
	printf("mb_type: quant=%d, mo_fw=%d, mo_bw=%d, pattern=%d, intra=%d\n",
		mb_quant, mb_mo_fw, mb_mo_bw, mb_pattern, mb_intra);
	// TODO:spatial_temporal_weight_code
	if(mb_mo_fw || mb_mo_bw)
	{
		if(picture_struct != PIC_STRUCT_FRAME)
			printf("field_mo_type: %d\n", field_mo_type = bs_gets(2));
		else if(frame_pred_frame_dct == 0)
		{
			int stwc = 0;	// TODO
			switch(frame_mo_type = bs_gets(2))
			{
			case 1: if(stwc <= 1) {
					mv_count = 1; mv_format = 'i'; dmv = 0;
					} else {
					mv_count = 1; mv_format = 'i'; dmv = 0; } break;
			case 2: mv_count = 1; mv_format = 'r'; dmv = 0; break;
			case 3: mv_count = 1; mv_format = 'i'; dmv = 1; break;
			default: return "illegal frame_mo_type";
			}
			printf("frame_mo_type: %d\n", frame_mo_type);
			if(frame_mo_type != 2)
				return "non-frame-based prediction is not supported";
		}
	}
	if(picture_struct == PIC_STRUCT_FRAME && frame_pred_frame_dct == 0
		&& (mb_intra || mb_pattern))
		printf("dct_type: %d\n", dct_type = bs_gets(1));
	return NULL;
}

static const char* motion_vectors(int s)
{
	if(mv_count == 1)
	{
		if(mv_format == 'i' && dmv != 1)
			mo_vert_field_select[0][s] = bs_gets(1);
		CALL(motion_vector(0, s));
	}
	else
	{
		mo_vert_field_select[0][s] = bs_gets(1);
		CALL(motion_vector(0, s));
		mo_vert_field_select[1][s] = bs_gets(1);
		CALL(motion_vector(1, s));
	}
	return NULL;
}

static const char* motion_vector(int r, int s)
{
	for(int t = 0; t < 2; ++t)
	{
		printf("mo_code[%d][%d][%d]: %d\n",
			r, s, t, mo_code[r][s][t] = bs_vlc(vlc_table_b10));
		if(f_code[s][t] != 1 && mo_code[r][s][t] != 0)
			printf("mo_residual[%d][%d][%d]: %d\n",
				r, s, t, mo_residual[r][s][t] = bs_get(f_code[s][t] - 1));
		if(dmv)
			printf("dmvector[%d]: %d\n",
				t, dmvector[t] = bs_vlc(vlc_table_b11));
	}
	return NULL;
}

static const char* coded_block_pattern()
{
	printf("cbp420: %d\n", cbp420 = bs_vlc(vlc_table_b9));
	if(chroma_fmt == 2)	// 4:2:2
		printf("cbp1: %d\n", cbp1 = bs_gets(2));
	if(chroma_fmt == 3)	// 4:4:4
		printf("cbp2: %d\n", cbp1 = bs_gets(6));

	for(int i = 0; i < 12; ++i) pattern_code[i] = mb_intra;
	if(mb_pattern)
	{
		for(int i = 0; i < 6; ++i) if(cbp420 & (1 << (5-i))) pattern_code[i] = 1;
		if(chroma_fmt == 2)	// 4:2:2
			for(int i = 6; i < 8; ++i)
				if(cbp1 & (1 << (7-i))) pattern_code[i] = 1;
		if(chroma_fmt == 3)	// 4:4:4
			for(int i = 6; i < 12; ++i)
				if(cbp2 & (1 << (11-i))) pattern_code[i] = 1;
	}
	printf("pattern_code:");
	for(int i = 0; i < block_count; ++i) printf(" %d", pattern_code[i]);
	printf("\n");
	return NULL;
}

static const char* block(int b)
{
	return NULL;
}

/*
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
	/-*
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
	*-/
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
*/


