//================================================================================
// m2v_dec_eval - MPEG2 ビデオ デコーダ
// ref: ISO13818-2
// $Id$
//================================================================================

#define _GNU_SOURCE
#define _USE_MATH_DEFINES
#include <stdio.h>
#include <string.h>
#include <malloc.h>
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
static const char* block_coefs(int b);
static const char* dequant(int b);
static const char* idct(int b);
static const char* mc();
static const char* output_yuv(int b);

static void alloc_yuv_buffers(int width, int height);
static void free_yuv_buffers();

// sequence_header
int horz_size, vert_size, aspect_ratio, frame_rate_code,
	frame_rate_n, frame_rate_d,
	bitrate_value, vbv_buf_size, const_param_flag;
int intra_qmat[8][8], nonintra_qmat[8][8];
int mb_width, mb_height;

// extension data
int seq_scalable;

// seq extension
int prof_and_level, prog_seq, chroma_fmt, low_delay, block_count;

// gop header
int time_code, closed_gop, broken_link;

// picture header
int npict, temp_ref, pic_coding_type, vbv_delay,
	full_pel_fw_vector, fw_f_code, full_pel_bw_vector, bw_f_code;

// picture coding extension
int f_code[2][2], intra_dc_precision, picture_struct, top_field_first,
	frame_pred_frame_dct, conceal_mv, q_scale_type, intra_vlc_fmt,
	alt_scan, rep_first_field, chroma_420_type, prog_frame,
	v_axis, field_seq, sub_carrier, burst_amp, sub_car_phase;

// slice
int nslice, max_slices, skip_slices;
int slice_vert_position;
int q_scale_code, intra_slice;
int mb_x, mb_y, prev_mb_addr;

// macroblock
int nmb;
int mb_addr_inc, mb_quant, mb_q_scale_code, mb_mo_fw, mb_mo_bw,
	mb_pattern, mb_intra;
int field_mo_type, frame_mo_type, dct_type, mv_count, mv_format, dmv;

// motion vectors
int mo_vert_field_select[2][2], mo_code[2][2][2], mo_residual[2][2][2],
	dmvector[2], PMV[2][2][2], vector[2][2][2];

// coded block pattern
int cbp420, cbp1, cbp2, pattern_code[12];

// block
int dct_dc_diff, dc_dct_pred[3];
int QFS[64], QF[8][8], F[8][8];

// yuv out buffer
int af;
int buf_width, buf_height;
int *fr_buf[2], *fr_u[2], *fr_v[2];
uint8_t *yuv_y, *yuv_cb, *yuv_cr;

// macroblock data
//
// B0 B1
// B2 B3
// C0 C1 (4:2:0)
// C2 C3 (4:2:2)
// C4 C5
// C6 C7 (4:4:4)
int f[8][8], p[8][8], d[8][8];

// idct (slow, but easy to understand)
static double IDCT_SLOW_COS[32][32];

static const char* const PCT_STRING = "0IPB4567";	// '4'=='D' if ISO11172-2
static const char* const CHROMA_FMT_NAME[] = {"reserved", "4:2:0", "4:2:2", "4:4:4"};
static const int CHROMA_FMT_MBH[] = {0, 24, 32, 48};
static const int BLK_COUNT[4] = {0, 6, 8, 12};
static const int ZIGZAG_SCAN[2][8][8] = {
	{{ 0,  1,  5,  6, 14, 15, 27, 28 },
	{  2,  4,  7, 13, 16, 26, 29, 42 },
	{  3,  8, 12, 17, 25, 30, 41, 43 },
	{  9, 11, 18, 24, 31, 40, 44, 53 },
	{ 10, 19, 23, 32, 39, 45, 52, 54 },
	{ 20, 22, 33, 38, 46, 51, 55, 60 },
	{ 21, 34, 37, 47, 50, 56, 59, 61 },
	{ 35, 36, 48, 49, 57, 58, 62, 63 }},
	{{ 0,  4,  6, 20, 22, 36, 38, 52 },
	{  1,  5,  7, 21, 23, 37, 39, 53 },
	{  2,  8, 19, 24, 34, 40, 50, 54 },
	{  3,  9, 18, 25, 35, 41, 51, 55 },
	{ 10, 17, 26, 30, 42, 46, 56, 60 },
	{ 11, 16, 27, 31, 43, 47, 57, 61 },
	{ 12, 15, 28, 32, 44, 48, 58, 62 },
	{ 13, 14, 29, 33, 45, 49, 59, 63 }},
};
static const int DEF_INTRA_QMAT[8][8] = {
	{  8, 16, 19, 22, 26, 27, 29, 34 },
	{ 16, 16, 22, 24, 27, 29, 34, 37 },
	{ 19, 22, 26, 27, 29, 34, 34, 38 },
	{ 22, 22, 26, 27, 29, 34, 37, 40 },
	{ 22, 26, 27, 29, 32, 35, 40, 48 },
	{ 26, 27, 29, 32, 35, 40, 48, 58 },
	{ 26, 27, 29, 34, 38, 46, 56, 69 },
	{ 27, 29, 35, 38, 46, 56, 69, 83 },
};
static const int INTRA_DC_MULT[4] = {8, 4, 2, 1};
static const int QUANT_SCALE[2][32] = {
{0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46,48,50,52,54,56,58,60,62},
{0,1,2,3,4,5,6,7,8,10,12,14,16,18,20,22,24,28,32,36,40,44,48,52,56,64,72,80,88,96,104,112}};

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

#define MV_FORMAT_FIELD		0
#define MV_FORMAT_FRAME		1

#define CLIP_S2048(x)		({ if((x) < -2048) (x) = -2048; else if((x) > 2047) (x) = 2047; })
#define CLIP_US255(x)		({ if((x) < 0) (x) = 0; else if((x) > 255) (x) = 255; })
#define CLIP_S256(x)		({ if((x) < -256) (x) = -256; else if((x) > 255) (x) = 255; })

static int iabs(int x)
{
	if(x < 0) return -x;
	return x;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// ビデオデコードトップ
//
const char* decode_video(const char* ref_dir, int slices, int skips)
{
	max_slices = slices;
	skip_slices = skips;
	nslice = 0;
	CALL(dump_init(ref_dir));

	// idctテーブルの初期化
	for(int i = 0; i < 16; ++i) for(int j = 0; j < 16; ++j)
	// for(int i = 0; i < 32; ++i) for(int j = 0; j < 32; ++j)
	{
		IDCT_SLOW_COS[i+16][j+16] =
		IDCT_SLOW_COS[i][j] =
			cos((double)i * M_PI / 16) * cos((double)j * M_PI / 16);
		IDCT_SLOW_COS[i+16][j] = IDCT_SLOW_COS[i][j+16] = -IDCT_SLOW_COS[i][j];
	}

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
			if(nslice == max_slices) break;
			n = bs_peek(32);
		}
		while(n == PICTURE_START_CODE || n == GROUP_START_CODE);
		if(nslice == max_slices) break;
	}
	while(n != SEQ_END_CODE);
	bs_get(32);
	dump_finish();
	free_yuv_buffers();
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// YUV データ用バッファの確保
//
static void alloc_yuv_buffers(int width, int height)
{
	buf_width = (width + 15) & ~15;
	buf_height = (height + 15) & ~15;

	free_yuv_buffers();
	yuv_y  = (uint8_t*)malloc(buf_width * buf_height);
	yuv_cb = (uint8_t*)malloc(buf_width * buf_height / 4);
	yuv_cr = (uint8_t*)malloc(buf_width * buf_height / 4);
	fr_buf[0] = (int*)malloc(buf_width * buf_height * 4 * sizeof(int));
	fr_buf[1] = fr_buf[0] + buf_width * buf_height * 2;
	for(int i = 0; i < 2; ++i)
	{
		fr_u[i] = fr_buf[i] + buf_width * buf_height;
		fr_v[i] = fr_u[i] + buf_width / 2;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// YUV データ用バッファの解放
//
static void free_yuv_buffers()
{
	if(yuv_y) free(yuv_y);
	if(yuv_cb) free(yuv_cb);
	if(yuv_cr) free(yuv_cr);
	if(fr_buf[0]) free(fr_buf[0]);
	yuv_y = yuv_cb = yuv_cr = NULL;
	for(int i = 0; i < 2; ++i) fr_buf[i] = fr_u[i] = fr_v[i] = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// シーケンスヘッダの取得
//
static const char* sequence_header()
{
	uint32_t n = bs_get(32);
	printf("---- SEQUENCE HEADER (0x%08x) ----\n", n);
	seq_scalable = 0;	// init
	if(n != SEQ_HEADER_CODE) return "illegal SEQ_HEADER_CODE";
	horz_size = bs_get(12);
	vert_size = bs_get(12);
	printf("size: %d x %d\n", horz_size, vert_size);
	mb_width = (horz_size + 15) / 16;
	mb_height = (vert_size + 15) / 16;

	alloc_yuv_buffers(horz_size, vert_size);
	af = 0;

	printf("mb: %d x %d (%d MBs)\n", mb_width, mb_height, mb_width * mb_height);
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
		int tmp[64];
		for(int i = 0; i < 64; ++i) tmp[i] = bs_gets(8);
		for(int v = 0; v < 8; ++v) for(int u = 0; u < 8; ++u)
		{
			printf(" %3d%s",
				intra_qmat[v][u] = tmp[ZIGZAG_SCAN[0][v][u]],
				u == 7 ? "\n" : "");
		}
	}
	else
	{
		memcpy(intra_qmat, DEF_INTRA_QMAT, sizeof(intra_qmat));
	}
	if(bs_gets(1) == 0b1)
	{
		printf("non_intra_quant_matrix:");
		int tmp[64];
		for(int i = 0; i < 64; ++i) tmp[i] = bs_gets(8);
		for(int v = 0; v < 8; ++v) for(int u = 0; u < 8; ++u)
		{
			printf(" %3d%s",
				nonintra_qmat[v][u] = tmp[ZIGZAG_SCAN[0][v][u]],
				u == 7 ? "\n" : "");
		}
	}
	else for(int i = 0; i < 64; ++i) nonintra_qmat[0][i] = 16;
	bs_align(8);
	bs_nextcode(0x000001, 3);
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// 拡張/ユーザーデータの取得(というか読み飛ばし)
//
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

////////////////////////////////////////////////////////////////////////////////////////////////////
// 拡張データの取得(サポートしない)
//
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

////////////////////////////////////////////////////////////////////////////////////////////////////
// ユーザーデータの読み飛ばし
//
static const char* user_data()
{
	uint32_t n = bs_get(32);
	printf("---- USER DATA (0x%08x) ----\n", n);
	if(n != UDATA_START_CODE) return "illegal UDATA_START_CODE";
	int i = bs_nextcode(0x000001, 3);
	printf("user_data: %d bytes\n", i);
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// シーケンス拡張情報の取得(MPEG2では必須)
//
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
	mb_width = (horz_size + 15) / 16;
	mb_height = (vert_size + 15) / 16;
	printf("bitrate w/ext: %u\n", bitrate_value += (bs_get(12) << 18));
	if(bs_gets(1) != 0b1) return "illegal marker-bit (seq-ext)";
	printf("vbv_buf_size w/ext: %u\n", vbv_buf_size += (bs_gets(8) << 10));
	printf("low_delay: %u\n", low_delay = bs_gets(1));
	frame_rate_n *= (bs_gets(2) + 1);
	frame_rate_d *= (bs_gets(5) + 1);
	printf("frame_rate w/ext: %lf (%u/%u)\n", (double)frame_rate_n / frame_rate_d,
		frame_rate_n, frame_rate_d);
	bs_align(8);
	n = bs_nextcode(0x000001, 3);
	if(n > 0) printf("*** skipped (seqext) %d ***\n", n);
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

////////////////////////////////////////////////////////////////////////////////////////////////////
// GOP ヘッダの取得(デコードにおいて重要な情報は無いので読み飛ばしでOK)
//
static const char* group_of_pictures_header()
{
	uint32_t n = bs_get(32);
	printf("---- GROUP OF PICTURE HEADER (0x%08x) ----\n", n);
	if(n != GROUP_START_CODE) return "illegal GROUP_START_CODE";
	printf("time_code: %u\n", time_code = bs_get(25));
	printf("drop_frame_flag: %u\n", (time_code >> 24) & 1);
	printf("time_code_hours: %u\n", (time_code >> 19) & 31);
	printf("time_code_minutes: %u\n", (time_code >> 13) & 63);
	printf("time_code_marker_bit: %u\n", (time_code >> 12) & 1);
	printf("time_code_seconds: %u\n", (time_code >> 6) & 63);
	printf("time_code_pictures: %u\n", (time_code >> 0) & 63);
	printf("closed_gop: %u\n", closed_gop = bs_gets(1));
	printf("broken_link: %u\n", broken_link = bs_gets(1));
	bs_align(8);
	n = bs_nextcode(0x000001, 3);
	if(n > 0) printf("**** skipped **** (gop) %d\n", n);
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// ピクチャヘッダの取得 (I,P,B の種別や f_code などが重要)
//
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
	n = bs_nextcode(0x000001, 3);
	if(n > 0) printf("**** skipped **** (picheader) %d\n", n);
	++npict;
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Picture Coding Extension
//
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
	n = bs_nextcode(0x000001, 3);
	if(n > 0) printf("**** skipped **** (pce) %d\n", n);
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// ピクチャーデータ本体
//
static const char* picture_data()
{
	uint32_t n;
	// memset(yuv_y, 16, horz_size * vert_size);
	// memset(yuv_cb, 128, horz_size * vert_size / 4);
	// memset(yuv_cr, 128, horz_size * vert_size / 4);
	do
	{
		CALL(slice());
		if(nslice == max_slices) return NULL;
		n = bs_peek(32);
	}
	while(0x00000101 <= n && n <= 0x000001af);
	if(dump_yuv)
	{
		fwrite(yuv_y, 1, horz_size * vert_size, dump_yuv);
		fwrite(yuv_cr, 1, horz_size * vert_size / 4, dump_yuv);
		fwrite(yuv_cb, 1, horz_size * vert_size / 4, dump_yuv);
	}
	af = 1 - af;
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// DC DCT 係数の初期化
//
static void reset_dc_dct_pred()
{
	dc_dct_pred[0] = dc_dct_pred[1] = dc_dct_pred[2] = (1 << (intra_dc_precision + 7));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// スライス
//
static const char* slice()
{
	uint32_t n = bs_get(32);
	if(nslice == skip_slices) dump_start();
	printf("---- SLICE (0x%08x, S:%04d) ----\n", n, nslice);
	if(n < 0x00000101 || n > 0x000001af) return "illegal slice start code";
	slice_vert_position = (n & 0xff) - 1;
	printf("slice_vert_position: %u\n", slice_vert_position);
	if(vert_size > 2800)
		return "video too large!"; // slice_vertical_position_extension
	if(seq_scalable)
		return "scalable is not supported!";
	printf("q_scale_code: %u\n", q_scale_code = bs_gets(5));
	if(q_scale_code == 0) return "illegal q_scale_code";
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
	prev_mb_addr = mb_width * mb_height - 1;
	// mb_x = 0;
	// mb_y = slice_vert_position;

	// ffmpeg の mpeg12.c L1683 によると、どうやらスライス先頭にも
	// MB のスキップ数が記録されるらしい(initial skip)。
	// このとき、テーブルは B1 を用いるが、インクリメント数-1 がスキップ数となる。
	// そうでないと、0が表現できないので。なお、エスケープは33でよい。
	// int init_skips = 0;
	// while(1)
	// {
	// 	int skips = bs_vlc(vlc_table_b1);
	// 	if(skips < 0)
	// 		init_skips += 33;
	// 	else if(skips == 0)
	// 		return "illegal End of slice (mb_addr_inc = eos)";
	// 	else
	// 	{
	// 		init_skips = skips - 1;
	// 		break;
	// 	}
	// }

	// printf("init_skips: %d\n", init_skips);
	// mb_x += init_skips;
	// if(mb_x >= mb_width)
	// 	return "initial skip overflow";

	reset_dc_dct_pred();
	memset(PMV, 0, sizeof(PMV));

	const char* r = NULL;
	do
	{
		r = macroblock();
		if(r) break;
	}
	while(bs_peek(23) != 0);
	if(r) printf("*** slice is skipeed because of error: %s ***\n", r);
	bs_align(8);
	n = bs_nextcode(0x000001, 3);
	if(n > 0) printf("*** skipped *** %u\n", n);
	++nslice;
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// マクロブロック
//
static const char* macroblock()
{
	mb_addr_inc = 0;
	while(1)
	{
		int inc = bs_vlc(vlc_table_b1);
		if(inc < 0)
			mb_addr_inc += 33;
		else if(inc == 0)
		{
			if(bs_peek(15) != 0) return "illegal end of slice (mb)";
			return NULL;	// End of slice
		}
		else
		{
			mb_addr_inc += inc;
			break;
		}
	}

	if(mb_addr_inc > 1 && pic_coding_type == 1)		// ffmpeg mpeg12.c L1817
		return "illegal MB skip in I frame!";
	if(mb_addr_inc > 1 && pic_coding_type == 2)
		memset(PMV, 0, sizeof(PMV));

	prev_mb_addr = (prev_mb_addr + mb_addr_inc) % (mb_width * mb_height);
	mb_x = prev_mb_addr % mb_width;
	mb_y = prev_mb_addr / mb_width + slice_vert_position;

	printf("---- MACROBLOCK (S:%05d, M:%04d) ----\n", nslice, nmb);
	printf("mb_addr_inc: %d\n", mb_addr_inc);
	printf("mb_addr: (%d, %d)\n", mb_x, mb_y);
	if(mb_addr_inc > 1) reset_dc_dct_pred();
	dump(dump_mb, NULL, "# slice %6d, mb %4d", nslice, nmb);
	dump(dump_mb, "mb_x mb_y mb_addr_inc", " %3d %3d %2d", mb_x, mb_y, mb_addr_inc);
	dump(dump_mb, "bytepos bitpos", "0x%x %d", g_total_bits / 8, g_total_bits % 8);
	CALL(macroblock_modes());
	if(mb_quant) printf("mb_q_scale_code: %d\n", mb_q_scale_code = bs_gets(5));
	else mb_q_scale_code = q_scale_code;
	if(mb_mo_fw || (mb_intra && conceal_mv)) CALL(motion_vectors(0));
	if(mb_mo_bw) CALL(motion_vectors(1));
	if(mb_intra && conceal_mv && bs_gets(1) != 0b1)
		return "illegal marker-bit (mb)";
	if((mb_intra && !conceal_mv) || (!mb_intra && !mb_mo_fw && pic_coding_type == 2))
		memset(PMV, 0, sizeof(PMV));
	for(int i = 0; i < 12; ++i) pattern_code[i] = mb_intra;
	if(mb_pattern) CALL(coded_block_pattern());

	dump(dump_rl, NULL, "# slice %6d, mb %4d", nslice, nmb);
	dump(dump_rl, "qs_type qs_code intra", " %d %2d %d",
		q_scale_type, mb_q_scale_code, mb_intra);

	for(int b = 0; b < block_count; ++b)
	{
		// block decode
		CALL(block(b));
	}

	++nmb;
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// マクロブロックモードのデコード
//
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
	if(!mb_intra) reset_dc_dct_pred();
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
					mv_count = 1; mv_format = MV_FORMAT_FIELD; dmv = 0;
					} else {
					mv_count = 1; mv_format = MV_FORMAT_FIELD; dmv = 0; } break;
			case 2: mv_count = 1; mv_format = MV_FORMAT_FRAME; dmv = 0; break;
			case 3: mv_count = 1; mv_format = MV_FORMAT_FIELD; dmv = 1; break;
			default: return "illegal frame_mo_type";
			}
			printf("frame_mo_type: %d\n", frame_mo_type);
			printf("mv_count: %d\n", mv_count);
			printf("mv_format: %s\n", mv_format == MV_FORMAT_FIELD ? "field" : "frame");
			printf("dmv: %d\n", dmv);
			if(frame_mo_type != 2)
				return "non-frame-based prediction is not supported";
		}
		else
		{
			mv_count = 1;
			mv_format = MV_FORMAT_FRAME;
			dmv = 0;
		}
	}
	if(picture_struct == PIC_STRUCT_FRAME && frame_pred_frame_dct == 0
		&& (mb_intra || mb_pattern))
		printf("dct_type: %d\n", dct_type = bs_gets(1));
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// マクロブロックの動きベクトルデコード
//
static const char* motion_vectors(int s)
{
	if(mv_count == 1)
	{
		if(mv_format == MV_FORMAT_FIELD && dmv != 1)
			mo_vert_field_select[0][s] = bs_gets(1);
		CALL(motion_vector(0, s));

		// モーションベクトルのデコード
		int r = 0;
		for(int t = 0; t < 2; ++t)
		{
			int r_size = f_code[s][t] - 1;
			int f = 1 << r_size;
			int high = (16 * f) - 1;
			int low = (-16 * f);
			int range = 32 * f;
			int delta;

			if(f == 1 || mo_code[r][s][t] == 0)
				delta = mo_code[r][s][t];
			else
			{
				delta = (iabs(mo_code[r][s][t] - 1) * f) + mo_residual[r][s][t] + 1;
				if(mo_code[r][s][t] < 0) delta = -delta;
			}

			if(delta < low || delta > high)
			{
				printf("delta is out of range: %d [%d:%d]\n", delta, low, high);
				return "mv_error";
			}

			int pred = PMV[r][s][t];
			if(mv_format == MV_FORMAT_FIELD && t == 1 && picture_struct == PIC_STRUCT_FRAME)
				pred /= 2;	// not used

			int x = pred + delta;
			if(x < low) x += range;
			if(x > high) x -= range;
			vector[r][s][t] = x;

			if(mv_format == MV_FORMAT_FIELD && t == 1 && picture_struct == PIC_STRUCT_FRAME)
				PMV[r][s][t] = x * 2;	// not used
			else
				PMV[r][s][t] = x;

			if(PMV[r][s][t]< low || PMV[r][s][t] > high)
			{
				printf("PMV is out of range: %d [%d:%d]\n", PMV[r][s][t], low, high);
				return "mv_error";
			}
		}
	}
	else
	{
		printf("mo_vert_field_select[0][%d]: %d\n", s,
			mo_vert_field_select[0][s] = bs_gets(1));
		CALL(motion_vector(0, s));
		printf("mo_vert_field_select[1][%d]: %d\n", s,
			mo_vert_field_select[1][s] = bs_gets(1));
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

////////////////////////////////////////////////////////////////////////////////////////////////////
// CBP (コードブロックパターン) のデコード
//
static const char* coded_block_pattern()
{
	printf("cbp420: %d\n", cbp420 = bs_vlc(vlc_table_b9));
	if(chroma_fmt == 2)	// 4:2:2
		printf("cbp1: %d\n", cbp1 = bs_gets(2));
	if(chroma_fmt == 3)	// 4:4:4
		printf("cbp2: %d\n", cbp1 = bs_gets(6));

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
	else for(int i = 0; i < 12; ++i) pattern_code[i] = 1;	// TODO: これでいいのか？要調査
	printf("pattern_code:");
	for(int i = 0; i < block_count; ++i) printf(" %d", pattern_code[i]);
	printf("\n");
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// 小ブロックの処理
//
static const char* block(int b)
{
	printf("---- BLOCK (S:%04d, M:%04d, B:%d) ----\n", nslice, nmb, b);

	// load coefficients
	CALL(block_coefs(b));

	// dequantisation
	CALL(dequant(b));

	// inverse DCT
	if(pattern_code[b]) CALL(idct(b));

	// motion compensation
	CALL(mc(b));

	// output yuv
	CALL(output_yuv(b));

	return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// RL 係数列のデコード
//
static const char* block_coefs(int b)
{
	if(!pattern_code[b])
	{
		memset(QF, 0, sizeof(QF));
		return NULL;
	}

	dump(dump_rl, NULL, "# slice %6d, mb %4d, block %2d", nslice, nmb, b);

	int i;
	const VLC_ENTRY* table_dct =
		(intra_vlc_fmt && mb_intra) ? vlc_table_b15 : vlc_table_b14;
	for(i = 0; i < 64; ++i) QFS[i] = 0;	// まず0で埋めておく
	i = 0;

	if(mb_intra)
	{
		int dct_diff;
		int dct_dc_size = bs_vlc(b < 4 ? vlc_table_b12 : vlc_table_b13);
		if(dct_dc_size > 0)
		{
			dct_dc_diff = bs_get(dct_dc_size);
			int half_range = 1 << (dct_dc_size - 1);
			if(dct_dc_diff >= half_range)
				dct_diff = dct_dc_diff;
			else
				dct_diff = (dct_dc_diff + 1) - (2 * half_range);
		}
		else dct_dc_diff = dct_diff = 0;

		int cc = (b < 4) ? 0 : (b % 2 + 1);

		// 先頭値(DC)
		QFS[0] = dc_dct_pred[cc] + dct_diff;
		printf("QFS[0]: %d, (%d + %d)\n", QFS[0], dc_dct_pred[cc], dct_diff);
		if(QFS[0] < 0 || QFS[0] >= (1 << (8 + intra_dc_precision)))
		{
			return "QFS[0] overflow!";
		}
		dc_dct_pred[cc] = QFS[i++];
		dump(dump_rl, NULL, " %2d %5d", 0, QFS[0]);
	}
	else if(bs_peek(1) == 0b1)
	{
		// non-intra の最初の係数で、先頭が 1 の場合(特例)
		// 11 -> (0,-1)
		// 10 -> (0,+1)
		if(bs_gets(2) & 1)
			QFS[i++] = -1;
		else
			QFS[i++] = 1;
		dump(dump_rl, NULL, " %2d %5d", 0, QFS[0]);
	}

	// のこりの係数
	while(1)
	{
		int c = bs_vlc(table_dct);
		// NOTE2 - “End of Block” shall not be the only code of the block.
		// の解釈がどうも分からない...
		// 「EOB だけのブロックは存在し得ない(EOBが最初に来ることはない)」
		// と思えばいいのか？→その解釈であっているようだ。
		int run, level;
		if(c == -1)
			break;	// End of block
		else if(c == -9)
		{
			// escape
			run = bs_gets(6);
			level = bs_get(12);
			if(level >= 2048) level -= 4096;
			if(level == -2048 || level == 0) return "level has forbidden value!";
			printf("### escape used\n");
		}
		else if(bs_gets(1))
		{
			// level is negative
			run = c >> 8; level = -(c & 255);
		}
		else
		{
			// level is positive
			run = c >> 8; level = c & 255;
		}
		if(i == 64) return "too much coefficients!";
		i += run;	// zero run
		if(i >= 64)
		{
			// static int lasterror = 0;
			// int k = g_total_bits / 8;
			// printf("run:%d\n 'invalid QFS index' at %d (+%d)\n", run, k, k - lasterror);
			// lasterror = k;
			// i = 63;
			return "invalid QFS index";
		}
		dump(dump_rl, NULL, " %2d %5d", run, level);
		QFS[i++] = level;
		// printf("QFS[%2d]: %d    (%d skipped)\n", i - 1, level, run);
	}
	dump(dump_rl, "eob", " %2d %5d", 0, 0);
	printf("QFS last: %d\n", i);
	dump(dump_qfs, NULL, "# slice %6d, mb %4d, block %2d", nslice, nmb, b);
	for(int j = 0; j <= (64 - 16); j += 16)
	{
		dump(dump_qfs, NULL, " %5d %5d %5d %5d %5d %5d %5d %5d %5d %5d %5d %5d %5d %5d %5d %5d",
			QFS[j+0], QFS[j+1], QFS[j+2], QFS[j+3],
			QFS[j+4], QFS[j+5], QFS[j+6], QFS[j+7],
			QFS[j+8], QFS[j+9], QFS[j+10], QFS[j+11],
			QFS[j+12], QFS[j+13], QFS[j+14], QFS[j+15]);
	}

	// inverse scan
	dump(dump_qf, NULL, "# slice %6d, mb %4d, block %2d", nslice, nmb, b);
	for(int v = 0; v < 8; ++v)
	{
		for(int u = 0; u < 8; ++u) QF[v][u] = QFS[ZIGZAG_SCAN[alt_scan][v][u]];
		dump(dump_qf, NULL, " %5d %5d %5d %5d %5d %5d %5d %5d",
			QF[v][0], QF[v][1], QF[v][2], QF[v][3],
			QF[v][4], QF[v][5], QF[v][6], QF[v][7]);
	}

	return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// 逆量子化
//
const char* dequant(int b)
{
	// F[v][u] = ((2×QF[v][u]+k)×W[w][v][u]×quant_scale)/32
	// where: k = 0              (intra)
	//            sign(QF[v][u]) (non-intra)

	int qs = QUANT_SCALE[q_scale_type][mb_q_scale_code];

	if(!pattern_code[b])
		memset(F, 0, sizeof(F));
	else if(mb_intra)
	{
		// intra

		// DC は定数乗算のみ
		F[0][0] = QF[0][0] * INTRA_DC_MULT[intra_dc_precision];

		// DC 以外
		for(int v = 0; v < 8; ++v) for(int u = (v > 0) ? 0 : 1; u < 8; ++u)
			F[v][u] = (2 * QF[v][u]) * intra_qmat[v][u] * qs / 32;
	}
	else
	{
		// non intra
		for(int v = 0; v < 8; ++v) for(int u = 0; u < 8; ++u)
		{
			int x = 2 * QF[v][u];
			if(x < 0) --x;
			if(x > 0) ++x;
			F[v][u] = x * nonintra_qmat[v][u] * qs / 32;
		}
	}

	dump(dump_lf, NULL, "# slice %6d, mb %4d, block %2d, qs %2d", nslice, nmb, b, qs);

	// clipping & mismatch control
	int sum = 0;		// RTL実装時は足す必要がなく、LSBだけ見ていればよい
	for(int v = 0; v < 8; ++v)
	{
		for(int u = 0; u < 8; ++u)
		{
			int x = F[v][u];
			CLIP_S2048(x);
			F[v][u] = x;
			sum += x;
		}

		if(v == 7 && !(sum & 1))
			F[7][7] ^= 1;	// if(x & 1) {x - 1} else {x + 1} と同義

		dump(dump_lf, NULL, " %5d %5d %5d %5d %5d %5d %5d %5d",
			F[v][0], F[v][1], F[v][2], F[v][3],
			F[v][4], F[v][5], F[v][6], F[v][7]);
	}

	return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IDCT
//
const char* idct(int b)
{
	//           2  N-1 N-1                    (2x+1)uπ     (2y+1)vπ
	// f(x,y) = --- Σ  Σ  C(u)C(v)F(u,v) cos --------- cos ---------
	//           N  u=0 v=0                        2N            2N
	// (N=8)

/*
	// /-*
	for(int y = 0; y < 8; ++y) for(int x = 0; x < 8; ++x)
	{
		double s = 0.0;
		for(int v = 0; v < 8; ++v) for(int u = 0; u < 8; ++u)
		{
			double d = (double)F[v][u] * IDCT_SLOW_COS[((2 * x + 1) * u) % 32][((2 * y + 1) * v) % 32];
			if(u == 0) d *= M_SQRT1_2;
			if(v == 0) d *= M_SQRT1_2;
			s += d;
		}
		s /= 2;
		if(s > 0) s = (s + 1) / 2;
		if(s < 0) s = (s - 1) / 2;
		// int i = (int)(s / 4);
		int i = (int)s;
		CLIP_S256(i);
		f[y+oy][x+ox] = i;
	}
	//-*/
	// extern void simple_idct(int L[8][8], int S[8][8]);
	// simple_idct(F, f);

/*
	dump(dump_sf, NULL, "# slice %6d, mb %4d, block %d",
		nslice, nmb, b);
	for(int y = 0; y < 8; ++y)
		dump(dump_sf, NULL, " %4d %4d %4d %4d %4d %4d %4d %4d",
			f[y+oy][0+ox], f[y+oy][1+ox], f[y+oy][2+ox], f[y+oy][3+ox],
			f[y+oy][4+ox], f[y+oy][5+ox], f[y+oy][6+ox], f[y+oy][7+ox]);
// */
// /*
	// extern void ff_simple_idct(int* block);
	extern void idct_hw(short* block);
	short f2[8][8];
	for(int i = 0; i < 64; ++i) f2[0][i] = F[0][i];
	// memcpy(f2, F, sizeof(f2));
	// ff_simple_idct((int*)f2);
	dump(dump_idwb, NULL, "# slice %6d, mb %4d, block %d",
		nslice, nmb, b);
	idct_hw((short*)f2);
	for(int y = 0; y < 8; ++y) for(int x = 0; x < 8; ++x) CLIP_S256(f2[y][x]);

	dump(dump_sf2, NULL, "# slice %6d, mb %4d, block %d",
		nslice, nmb, b);
	for(int y = 0; y < 8; ++y)
		dump(dump_sf2, NULL, " %4d %4d %4d %4d %4d %4d %4d %4d",
			f[y][0] = f2[y][0], f[y][1] = f2[y][1],
			f[y][2] = f2[y][2], f[y][3] = f2[y][3],
			f[y][4] = f2[y][4], f[y][5] = f2[y][5],
			f[y][6] = f2[y][6], f[y][7] = f2[y][7]);
// */
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// 動き補償
//
const char* mc(int b)
{
/*
	p.104 vectorの数
	r : マクロブロック中でいくつめか、ということらしいが
	    要するに16x8とか使わない場合は無視していいのか？
	s : 予測方向(0:forward,1:backward)
	t : 座標方向(0:水平,1:垂直)

	予測の種類
	通常予測: 16x16 の MB 単位で予測
	半MB予測: 16x8 の単位で予測。仕様書ではfield pictureについてしか触れていないので
	          対応しない。
	DualPrime: フルサイズのvector１つ＋差分小のvector３つ、計4つのvectorから
	           予測する。frame picturesでも使えるらしいが、あくまでfield予測用である。
			   ので、やっぱり対応いらない？

*/
	memset(p[b], 0, sizeof(p[b]));
	if(mb_intra)
	{
		// イントラの場合
		// for(int y = 0; y < 8; ++y) for(int x = 0; x < 8; ++x)
		// 	d[b][y][x] = f[b][y][x];
		if(conceal_mv) fprintf(stderr, "conceal_mv!\n");
		return NULL;
	}

	if(mb_mo_bw) return "backward is not supported";
	// if(!mb_mo_fw)
	// {
	// 	// fprintf(stderr, "no mb_mo_fw\n");
	// 	return NULL;
	// }

	int ivx, ivy, halfx, halfy;
	ivx = vector[0][0][0];
	if(b >= 4) ivx /= 2;
	halfx = ivx & 1;
	ivx = (ivx & ~1) / 2;
	ivy = vector[0][0][1];
	if(b >= 4) ivy /= 2;
	halfy = ivy & 1;
	ivy = (ivy & ~1) / 2;

	ivx += (b < 4) ? (mb_x * 16 + (b & 1) * 8) : (mb_x * 8);
	ivy += (b < 4) ? (mb_y * 16 + (b & 2) * 4) : (mb_y * 8);

	int rf = 1 - af;
	int* buf = (b < 4) ? fr_buf[rf] : (b == 4) ? fr_u[rf] : fr_v[rf];
#define BUF(y, x)	(buf[(y) * buf_width + (x)])

	for(int y = 0, oy = ivy; y < 8; ++y, ++oy) for(int x = 0, ox = ivx; x < 8; ++x, ++ox)
	{
		if(!halfx && !halfy)
			// 整数精度のみ
			p[y][x] = BUF(oy, ox);
		else if(!halfx && halfy)
			// 垂直方向だけ半画素精度
			p[y][x] = (BUF(oy, ox) + BUF(oy+1, ox) + 1) / 2;
		else if(halfx && !halfy)
			// 水平方向だけ半画素精度
			p[y][x] = (BUF(oy, ox) + BUF(oy, ox+1) + 1) / 2;
		else
			// 水平垂直ともに半画素精度
			p[y][x] = (BUF(oy, ox) + BUF(oy, ox+1)
					 + BUF(oy+1, ox) + BUF(oy+1, ox+1) + 2) / 4;
	}
#undef BUF

	return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// YUV バッファへの出力
//
static const char* output_yuv(int b)
{
	int bx = (b < 4) ? (mb_x * 16 + (b & 1) * 8) : (mb_x * 8);
	int by = (b < 4) ? (mb_y * 16 + (b & 2) * 4) : (mb_y * 8);
	int i;
	int* buf = (b < 4) ? fr_buf[af] : (b == 4) ? fr_u[af] : fr_v[af];

	for(int y = 0; y < 8; ++y) for(int x = 0; x < 8; ++x)
	{
		i = f[y][x] + p[y][x];
		CLIP_US255(i);
		buf[(by+y)*buf_width+(bx+x)] = d[y][x] = i;
		if(b < 4) i = d[y][x] * 224 / 256 + 16;
		else      i = d[y][x] * 219 / 256 + 16;
		CLIP_US255(i);
		if(b < 4)		yuv_y [(by+y)*buf_width+(bx+x)] = i;
		else if(b == 5)	yuv_cb[(by+y)*buf_width/2+(bx+x)] = i;
		else			yuv_cr[(by+y)*buf_width/2+(bx+x)] = i;
	}

	return NULL;
}

