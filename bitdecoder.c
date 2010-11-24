//================================================================================
// m2v_dec_eval - MPEG2 ビデオストリーム ビットデコーダ
// ref: ISO13818-2
// $Id$
//================================================================================

#include "m2v_dec_eval.h"
#include "bitreader.h"
#include <signal.h>
#include <string.h>

static void sigint_handler();
static void next_header(bitstream* bs);
static int sequence_header(bitstream* bs);
static int extension_and_user_data(bitstream* bs);
static int sequence_extension(bitstream* bs);
static int gop_header(bitstream* bs);
static int picture_header(bitstream* bs);
static int picture_coding_extension(bitstream* bs);
static int picture_data(bitstream* bs);
static int slice(bitstream* bs);
static int macroblock(bitstream* bs);
static int block(bitstream* bs, int b);

#define PICTURE_START_CODE	0x00000100
#define UDATA_START_CODE	0x000001b2
#define SEQ_HEADER_CODE		0x000001b3
#define SEQ_ERROR_CODE		0x000001b4
#define EXT_START_CODE		0x000001b5
#define SEQ_END_CODE		0x000001b7
#define GROUP_START_CODE	0x000001b8

#define SEQ_EXT_ID			0b0001
#define PIC_CODE_EXT_ID		0b1000

#define CALL(x)		({ if(!(x)) return 0; })

static int stop_decode = 0;

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

////////////////////////////////////////////////////////////////////////////////
/// @brief ビットデコード実行
///
/// ビットストリームをデコードし、後段の呼び出しを行う。
///
int bitdecode(bitstream* bs)
{
	signal(SIGINT, sigint_handler);

	uint32_t n;
	next_header(bs);

	do
	{
		CALL(sequence_header(bs));
		next_header(bs);
		CALL(sequence_extension(bs));
		CALL(mc_allocbuffer(mb_wd, mb_ht));
		CALL(extension_and_user_data(bs));
		do
		{
			if(bs_peek(bs, 32) == GROUP_START_CODE)
			{
				CALL(gop_header(bs));
				CALL(extension_and_user_data(bs));
			}
			CALL(picture_header(bs));
			CALL(picture_coding_extension(bs));
			CALL(extension_and_user_data(bs));
			CALL(picture_data(bs));
			n = bs_peek(bs, 32);
			if(stop_decode) break;
		}
		while(n == PICTURE_START_CODE || n == GROUP_START_CODE);
		if(stop_decode) break;
	}
	while(n != SEQ_END_CODE);

	CALL(mc_allocbuffer(0, 0));
	signal(SIGINT, SIG_DFL);
	return 1;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief Ctrl+C のハンドラ
///
/// フラグを立て、次のピクチャ境界でデコードを停止させる。
///
static void sigint_handler()
{
	++stop_decode;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief 次のヘッダへ移動
///
/// バイト境界へ移動した上で、ヘッダの先頭バイト 0x000001 が見つかるまで
/// ジャンプする。
/// バイト境界移動以外のジャンプが発生した場合(つまりゴミバイトがあった場合)
/// エラー出力にメッセージを出力する。
///
static void next_header(bitstream* bs)
{
	bs_align(bs, 8);
	int i;
	for(i = 0; bs_peek(bs, 24) != 0x000001; ++i);
	if(i > 0) printf("[%d bytes skipped]\n", i);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief シーケンスヘッダを取得
///
static int sequence_header(bitstream* bs)
{
	if(bs_get(bs, 32) != SEQ_HEADER_CODE)
	{
		printf("Error: Illegal Sequence Header\n");
		return 0;
	}

	video_wd = bs_get(bs, 12);
	video_ht = bs_get(bs, 12);
	mb_wd = (video_wd + 15) >> 4;
	mb_ht = (video_ht + 15) >> 4;
	printf("Video Size: %dx%d, (%dx%d MBs)\n",
		video_wd, video_ht, mb_wd, mb_ht);
	aspect_ratio = bs_get(bs, 4);
	frame_rate_code = bs_get(bs, 4);
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
	default:
		printf("Error: Illegal Frame Rate Code!\n");
		return 0;
	}
	printf("Frame Rate: %d (%d/%d)\n", frame_rate_code,
		frame_rate_n, frame_rate_d);
	bitrate_value = bs_get(bs, 18);
	printf("Bitrate: %d\n", bitrate_value);
	vbv_buf_size = bs_get(bs, 10);
	const_param_flag = bs_get(bs, 1);
	printf("Intra Quant Matrix: ");
	if(bs_get(bs, 1))
	{
		printf("original\n");
		int tmp[64];
		for(int i = 0; i < 64; ++i) tmp[i] = bs_get(bs, 8);
		for(int v = 0; v < 8; ++v) for(int u = 0; u < 8; ++u)
			intra_qmat[v][u] = tmp[ZIGZAG_SCAN[0][v][u]];
	}
	else
	{
		printf("default\n");
		memcpy(intra_qmat, DEF_INTRA_QMAT, sizeof(intra_qmat));
	}
	printf("Non-Intra Quant Matrix: ");
	if(bs_get(bs, 1))
	{
		printf("original\n");
		int tmp[64];
		for(int i = 0; i < 64; ++i) tmp[i] = bs_get(bs, 8);
		for(int v = 0; v < 8; ++v) for(int u = 0; u < 8; ++u)
			nonintra_qmat[v][u] = tmp[ZIGZAG_SCAN[0][v][u]];
	}
	else
	{
		printf("default\n");
		for(int i = 0; i < 64; ++i) nonintra_qmat[0][i] = 16;
	}
	return 1;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief シーケンス拡張を取得
///
/// MPEG2 ではシーケンスヘッダの直後にくるはず。
/// (逆に、直後にこれが無かったら MPEG1 扱いにするのが正しい)
///
static int sequence_extension(bitstream* bs)
{
	if(bs_get(bs, 32) != EXT_START_CODE || bs_get(bs, 4) != SEQ_EXT_ID)
	{
		printf("Error: Illegal Sequence Extension Header\n");
		return 0;
	}

	static const char* profs[] = {"---", "High", "Spatially Scalable",
		"SNR Scalable", "Main", "Simple", "---", "---"};
	static const char* levels[] = {"---", "---", "High", "High 1440",
		"Main", "Low", "---", "---"};

	prof_and_level = bs_get(bs, 8);
	printf("Profile: %s Profile %s Level\n",
		profs[prof_and_level & 0x80 ? 0 : (prof_and_level >> 4) & 7],
		levels[(prof_and_level & 0x80 || prof_and_level & 1) ? 0
			: (prof_and_level >> 1) & 7]);
	prog_seq = bs_get(bs, 1);
	if(!prog_seq)
	{
		printf("Error: Interlace is not supported!\n");
		return 0;
	}
	chroma_fmt = bs_get(bs, 2);
	if(bs_peek(bs, 4) != 0)
	{
		video_wd += (bs_get(bs, 2) << 12),
		mb_wd = (video_wd + 15) >> 4;
		video_ht += (bs_get(bs, 2) << 12),
		mb_ht = (video_ht + 15) >> 4;
		printf("Video Size (w/ext): %dx%d (%dx%d MBs)\n",
			video_wd, video_ht, mb_wd, mb_ht);
	}
	else bs_get(bs, 4);
	if(bs_peek(bs, 12) != 0)
	{
		bitrate_value += (bs_get(bs, 12) << 18);
		printf("Bitrate (w/ext): %d\n", bitrate_value);
	}
	else bs_get(bs, 12);
	if(!bs_get(bs, 1))
	{
		printf("Error: Illegal Marker Bit (in sequence_extension)\n");
		return 0;
	}
	vbv_buf_size += (bs_get(bs, 8) << 10);
	low_delay = bs_get(bs, 1);
	if(bs_peek(bs, 7) != 0)
	{
		frame_rate_n *= (bs_get(bs, 2) + 1);
		frame_rate_d *= (bs_get(bs, 5) + 1);
		printf("Frame Rate (w/ext): %d/%d\n",
			frame_rate_n, frame_rate_d);
	}
	else bs_get(bs, 7);
	bs_align(bs, 8);

	return 1;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief 拡張情報やユーザーデータの取得(読み飛ばし)
///
/// 使わない内容なので読み飛ばすだけ。
/// ただしシーケンス拡張やピクチャ拡張は別関数で読み取る。
///
static int extension_and_user_data(bitstream* bs)
{
	uint32_t n = bs_peek(bs, 32);
	while(n == EXT_START_CODE || n == UDATA_START_CODE)
	{
		bs_get(bs, 32);
		next_header(bs);
	}
	return 1;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief GOP ヘッダの取得
///
/// 使わない内容なので読み飛ばすだけ。
///
static int gop_header(bitstream* bs)
{
	if(bs_get(bs, 32) != GROUP_START_CODE)
	{
		printf("Error: Illegal GOP Header\n");
		return 0;
	}
	time_code = bs_get(bs, 25);
	closed_gop = bs_get(bs, 1);
	broken_link = bs_get(bs, 1);
	bs_align(bs, 8);
	return 1;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief ピクチャヘッダの取得
///
static int picture_header(bitstream* bs)
{
	if(bs_get(bs, 32) != PICTURE_START_CODE)
	{
		printf("Error: Illegal Picture Header\n");
		return 0;
	}
	temp_ref = bs_get(bs, 10);
	pic_coding_type = bs_get(bs, 3);
	if(pic_coding_type < 1 || pic_coding_type > 2)
	{
		printf("Error: Only I/P Frames are supported!\n");
		return 0;
	}
	vbv_delay = bs_get(bs, 16);
	if(pic_coding_type & 2 && (bs_get(bs, 1) != 0 || bs_get(bs, 3) != 7))
	{
		printf("Error: MPEG1 is not supported!\n");
		return 0;
	}
	// if(pic_coding_type == 3 && (bs_get(bs, 1) != 0 || bs_get(bs, 3) != 7))
	// 	return 0;
	while(bs_get(bs, 1)) bs_get(bs, 8);
	bs_align(bs, 8);
	return 1;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief ピクチャ拡張の取得
///
static int picture_coding_extension(bitstream* bs)
{
	if(bs_get(bs, 32) != EXT_START_CODE && bs_get(bs, 4) != PIC_CODE_EXT_ID)
	{
		printf("Error: Illegal Picture Coding Extension Header");
		return 0;
	}

	f_code[0][0] = bs_get(bs, 4);
	f_code[0][1] = bs_get(bs, 4);
	if(bs_get(bs, 8) != 0xff)
	{
		printf("Error: B Frame is not supported!\n");
		return 0;
	}
	intra_dc_precision = bs_get(bs, 2);
	picture_struct = bs_get(bs, 2);
	top_field_first = bs_get(bs, 1);
	frame_pred_frame_dct = bs_get(bs, 1);
	conceal_mv = bs_get(bs, 1);
	q_scale_type = bs_get(bs, 1);
	intra_vlc_fmt = bs_get(bs, 1);
	alt_scan = bs_get(bs, 1);
	if(alt_scan)
	{
		printf("Error: Alternate Scan is not supported!\n");
		return 0;
	}
	rep_first_field = bs_get(bs, 1);
	chroma_420_type = bs_get(bs, 1);
	prog_frame = bs_get(bs, 1);
	if(!prog_frame)
	{
		printf("Error: Interlace is not supported!\n");
		return 0;
	}
	if(bs_get(bs, 1)) bs_get(bs, 1 + 3 + 1 + 7 + 8);
	bs_align(bs, 8);
	return 1;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief ピクチャデータ(中身)のデコード
///
static int picture_data(bitstream* bs)
{
	// バッファの切り替え
	return 0;
}


