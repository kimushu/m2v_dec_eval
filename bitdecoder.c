//================================================================================
// m2v_dec_eval - MPEG2 ビデオストリーム ビットデコーダ
// ref: ISO13818-2
// $Id$
//================================================================================

#include "m2v_dec.h"
#include "bitreader.h"

////////////////////////////////////////////////////////////////////////////////
// @brief ビットデコード実行
//
// 指定したビットストリームに対し、デコードを実行する。なお、読み込みの単位は
// シーケンスヘッダ・ピクチャヘッダ・マクロブロックのいずれかとなる。
// 読み込まれたデータ(or終端)は戻り値で返される。
//
// @param bs デコード対象のビットストリーム
//
// @retval BD_RET_ERROR      エラーを検出
// @retval BD_RET_SKIP       次のヘッダまでスキップ発生(ごみ検出)
// @retval BD_RET_SEQUENCE   シーケンスヘッダを読み込み (解像度などが有効)
// @retval BD_RET_IGNORE     無視してよいヘッダを検出(UserDataなど)
// @retval BD_RET_PICTURE    ピクチャヘッダを読み込み ()
// @retval BD_RET_SLICE      スライスヘッダを読み込み ()
// @retval BD_RET_MACROBLOCK マクロブロックを読み込み (QFS が有効)
// @retval BD_RET_EOF        終端に到達
//
int bitdecode(bitstream* bs)
{
	if(!bs_aligned(bs, 8) || bs_peek(bs, 24) != 0x000001)
	{
		bs_align(bs, 8);
		do { bs_get(bs, 8); } while (bs_peek(bs, 24) != 0x000001);
		return BD_RET_SKIP;
	}

	switch(bs_get(bs, 32))
	{
	case 0x000001b3:	// SEQ_HEADER_CODE
		video_wd = bs_get(bs, 12);
		video_ht = bs_get(bs, 12);
		mb_wd = (video_wd + 15) >> 4;
		mb_ht = (video_ht + 15) >> 4;
		fprintf(stderr, "Video Size: %dx%d, (%dx%d MBs)\n",
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
			fprintf(stderr, "Illegal Frame Rate Code!\n");
			return BD_RET_ERROR;
		}
		fprintf(stderr, "Frame Rate: %d (%d/%d)\n", frame_rate_code,
			frame_rate_n, frame_rate_d);
		bitrate_value = bs_get(bs, 18);
		fprintf(stderr, "Bitrate: %d\n", bitrate_value);
		vbv_buf_size = bs_get(bs, 10);
		const_param_flag = bs_get(bs, 1);
		fprintf(stderr, "Intra Quant Matrix: ");
		if(bs_get(bs, 1))
		{
			fprintf(stderr, "original\n");
			int tmp[64];
			for(int i = 0; i < 64; ++i) tmp[i] = bs_get(bs, 8);
			for(int v = 0; v < 8; ++v) for(int u = 0; u < 8; ++u)
				intra_qmat[v][u] = tmp[ZIGZAG_SCAN[0][v][u]];
		}
		else
		{
			fprintf(stderr, "default\n");
			memcpy(intra_qmat, DEF_INTRA_QMAT, sizeof(intra_qmat));
		}
		fprintf(stderr, "Non-Intra Quant Matrix: ");
		if(bs_get(bs, 1))
		{
			fprintf(stderr, "original\n");
			int tmp[64];
			for(int i = 0; i < 64; ++i) tmp[i] = bs_get(bs, 8);
			for(int v = 0; v < 8; ++v) for(int u = 0; u < 8; ++u)
				nonintra_qmat[v][u] = tmp[ZIGZAG_SCAN[0][v][u]];
		}
		else
		{
			fprintf(stderr, "default\n");
			for(int i = 0; i < 64; ++i) nonintra_qmat[0][i] = 16;
		}
		bs_align(bs, 8);
		// TODO:規格上は、ここでスタートコード検索もするんだが
		if(bs_get(bs, 32) != 0x000001b5 || bs_get(bs, 4) != 0b0001)
			return BD_RET_ERROR;
		prof_and_level = bs_get(bs, 8);
		fprintf(stderr, "Profile: %d\n", prof_and_level);
		prog_seq = bs_get(bs, 1);
		chroma_fmt = bs_get(bs, 2);
		video_extwd = bs_get(bs, 2);
		video_wd += (video_extwd << 12),
		mb_wd = (video_wd + 15) >> 4;
		video_extht = bs_get(bs, 2);
		video_ht += (video_extht << 12),
		mb_ht = (video_ht + 15) >> 4;
		if(video_extwd > 0 || video_extht > 0)
			fprintf(stderr, "Video Size (w/ext): %dx%d (%dx%d MBs)\n",
				video_wd, video_ht, mb_wd, mb_ht);
		bitrate_ext = bs_get(bs, 12);
		if(bitrate_ext > 0)
			fprintf(stderr, "Bitrate (w/ext): %d\n",
				bitrate_value += (bitrate_ext << 18));
		if(!bs_get(bs, 1)) return BD_RET_ERROR;
		vbv_buf_size += (bs_get(bs, 8) << 10);
		low_delay = bs_get(bs, 1);
		if(bs_peek(bs, 7) != 0)
		{
			frame_rate_n *= (bs_get(bs, 2) + 1);
			frame_rate_d *= (bs_get(bs, 5) + 1);
			fprintf(stderr, "Frame Rate (w/ext): %d/%d\n",
				frame_rate_n, frame_rate_d);
		}
		else bs_get(bs, 7);
		bs_align(bs, 8);
		return BD_RET_SEQUENCE;

	case 0x000001b5:	// EXT_START_CODE
	case 0x000001b2:	// UDATA_START_CODE
	case 0x000001b8:	// GROUP_START_CODE
		do { bs_get(bs, 8); } while(bs_peek(bs, 24) != 0x000001);
		return BD_RET_IGNORE;

	case 0x00000100:	// PICTURE_START_CODE
		temp_ref = bs_get(bs, 10);
		pic_coding_type = bs_get(bs, 3);
		if(pic_coding_type < 1 || pic_coding_type > 2) return BD_RET_ERROR;
		vbv_delay = bs_get(bs, 16);
		if(pic_coding_type & 2 && (bs_get(bs, 1) != 0 || bs_get(bs, 3) != 7))
			return BD_RET_ERROR;	// MPEG2
		// if(pic_coding_type == 3 && (bs_get(bs, 1) != 0 || bs_get(bs, 3) != 7))
		// 	return BD_RET_ERROR;	// MPEG2
		while(bs_get(bs, 1)) bs_get(bs, 8);
		bs_align(bs, 8);
		// TODO:規格上は、ここでスタートコード検索もするんだが
		if(bs_get(bs, 32) != 0x000001b5 && bs_get(bs, 4) != 0b1000)
			return BD_RET_ERROR;	// mismatch EXT_START_CODE / PIC_CODE_EXT_ID
		f_code[0][0] = bs_get(bs, 4);
		f_code[0][1] = bs_get(bs, 4);
		if(bs_get(bs, 8) != 0xff) return BD_RET_ERROR;	// 'B' is not supported
		intra_dc_precision = bs_get(bs, 2);
		picture_struct = bs_get(bs, 2);
		top_field_first = bs_get(bs, 1);
		frame_pred_frame_dct = bs_get(bs, 1);
		conceal_mv = bs_get(bs, 1);
		q_scale_type = bs_get(bs, 1);
		intra_vlc_fmt = bs_get(bs, 1);
		alt_scan = bs_get(bs, 1);
		if(alt_scan) return BD_RET_ERROR;	// interlace is not supported
		rep_first_field = bs_get(bs, 1);
		chroma_420_type = bs_get(bs, 1);
		prog_frame = bs_get(bs, 1);
		if(!prog_frame) return BD_RET_ERROR;	// progressive only
		if(bs_get(bs, 1)) bs_get(bs, 1 + 3 + 1 + 7 + 8);
		bs_align(bs, 8);
		return BD_RET_PICTURE;
	}

	if()
}

