//================================================================================
// m2v_dec_eval - MPEG2 ビデオストリーム ビットデコーダ
// ref: ISO13818-2
// $Id: bitdecoder.c 98 2012-04-07 03:52:11Z kimu_shu $
//================================================================================

#include "m2v_dec.h"
#include "bitreader.h"
#include "dump.h"
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

static void dump_seq();
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
static int block_coefs(bitstream* bs, int b);
static void reset_dct_dc_pred();

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

/**================================================================================
 * @brief ビットデコード実行
 *
 * ビットストリームをデコードし、後段の呼び出しを行う。
 *
 * @param bs       ビットストリーム
 */
int bitdecode(bitstream* bs)
{
	if(!vlc_init_table()) return 0;
	signal(SIGINT, sigint_handler);
	npict = nslice = 0;
	nseq = 0;

	int cycle_total = 0, cycle_picts = 0, cycle_peak = 0;
	int remainder_pictures = max_pictures;

	uint32_t n;
	next_header(bs);

	do
	{
		if(nseq == start_seq)
		{
			char header[PATH_MAX];
			printf("---- START DUMP ----\n");
			sprintf(header, "# input=\"%s\"\n# vim:ts=8\n", input);
			dump_start();
			dump_seq();
			dumpx_bitstream("%s", header);
			dumpx_side("%s", header);
			dumpx_rl("%s", header);
			dumpx_mc_fetch("%s", header);
			dumpx_mc_mix("%s", header);
			dumpx_fptr("%s", header);
			dumpx_rgb("%s", header);
		}
		CALL(sequence_header(bs));
		next_header(bs);
		CALL(sequence_extension(bs));
		CALL(mc_allocbuffer());
		CALL(extension_and_user_data(bs));
		do
		{
			if(bs_peek(bs, 32) == GROUP_START_CODE)
			{
				CALL(gop_header(bs));
				CALL(extension_and_user_data(bs));
			}
			cycle_esti = 0;
			CALL(picture_header(bs));
			CALL(picture_coding_extension(bs));
			if(verbose_mode) printf("[Picture %5d] %c\n",
					npict, pic_coding_type == 1 ? 'I' : 'P');
			CALL(extension_and_user_data(bs));
			CALL(picture_data(bs));
			dumpx_side("PICE\t%d\n", npict);
			n = bs_peek(bs, 32);
			++npict;
			if(nseq >= start_seq && --remainder_pictures == 0) stop_decode = 1;
			printf("Picture #%d: %d cycles\n", npict, cycle_esti);
			if(cycle_peak < cycle_esti) cycle_peak = cycle_esti;
			cycle_total += cycle_esti;
			++cycle_picts;
			if(stop_decode) break;
		}
		while(n == PICTURE_START_CODE || n == GROUP_START_CODE);
		if(end_seq >= start_seq && nseq == end_seq) stop_decode = 1;
		if(stop_decode) break;
		++nseq;
	}
	while(n != SEQ_END_CODE);

	printf(
	"----------------------------------------\n"
	" Cycle Estimation\n"
	"   Avg. %6d\n"
	"   Peak %6d\n"
	"----------------------------------------\n",
	(cycle_total / cycle_picts), cycle_peak);

	mc_freebuffer();
	signal(SIGINT, SIG_DFL);
	return 1;
}

/**================================================================================
 * @brief ダンプの MB/Block ヘッダを出力
 */
void dump_header(FILE* fp)
{
	if(nblock >= 0)
	{
		// dump(fp, NULL, "# Picture %5d, Slice %6d, MB %3d, Block %d",
		// 		npict, nslice, nmb, nblock);
		dump(fp, NULL, " %5d %6d %3d %d # P=%d,S=%d,MB=%d,Blk=%d",
			npict, nslice, nmb, nblock,
			npict, nslice, nmb, nblock);
	}
	else
	{
		// dump(fp, NULL, "# Picture %5d, Slice %6d, MB %3d",
		// 		npict, nslice, nmb);
		dump(fp, NULL, " %5d %6d %3d # P=%d,S=%d,MB=%d",
			npict, nslice, nmb, npict, nslice, nmb);
	}
}

/**================================================================================
 * @brief シーケンス情報のダンプ
 */
void dump_seq()
{
	dump(dump_sequence, NULL, " %5d # sequence", nseq);
	dump(dump_sequence, "width height", " %4d %4d", video_wd, video_ht);
	dump(dump_sequence, "mb_wd mb_ht", " %2d %2d", mb_wd, mb_ht);
	dump(dump_sequence, "fr_n / fr_d", " %5d %4d", frame_rate_n, frame_rate_d);
	dump(dump_parser, NULL, "SQ %4d", nseq);
	dump(dump_sequence, "cust_qm_intra", " %d", cust_qm_intra);
	dump(dump_parser, NULL, "%1d # cust_qm_intra", cust_qm_intra);
	if(cust_qm_intra)
	{
		int* p = custom_qmat[1];
		for(int i = 0; i < 8; ++i, p += 8)
		{
			dump(dump_sequence, NULL, " %3d %3d %3d %3d %3d %3d %3d %3d",
				p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
			dump(dump_parser, NULL, "%3d %3d %3d %3d # %2d - %2d",
				p[0], p[1], p[2], p[3], i * 8, i * 8 + 3);
			dump(dump_parser, NULL, "%3d %3d %3d %3d # %2d - %2d",
				p[4], p[5], p[6], p[7], i * 8 + 4, i * 8 + 7);
		}
	}
	dump(dump_sequence, "cust_qm_nonintra", " %d", cust_qm_nonintra);
	dump(dump_parser, NULL, "%1d # cust_qm_nonintra", cust_qm_nonintra);
	if(cust_qm_nonintra)
	{
		int* p = custom_qmat[0];
		for(int i = 0; i < 8; ++i, p += 8)
		{
			dump(dump_sequence, NULL, " %3d %3d %3d %3d %3d %3d %3d %3d",
				p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
			dump(dump_parser, NULL, "%3d %3d %3d %3d # %2d - %2d",
				p[0], p[1], p[2], p[3], i * 8, i * 8 + 3);
			dump(dump_parser, NULL, "%3d %3d %3d %3d # %2d - %2d",
				p[4], p[5], p[6], p[7], i * 8 + 4, i * 8 + 7);
		}
	}
}

/**================================================================================
 * @brief Ctrl+C のハンドラ
 *
 * フラグを立て、次のピクチャ境界でデコードを停止させる。
 */
static void sigint_handler()
{
	if(++stop_decode > 3) abort();
}

/**================================================================================
 * @brief 次のヘッダへ移動
 *
 * バイト境界へ移動した上で、ヘッダの先頭バイト 0x000001 が見つかるまで
 * ジャンプする。
 * バイト境界移動以外のジャンプが発生した場合(つまりゴミバイトがあった場合)
 * エラー出力にメッセージを出力する。
 */
static void next_header(bitstream* bs)
{
	bs_align(bs, 8);
	bs_get(bs, 0, "NEXH");
	int i;
	for(i = 0; bs_peek(bs, 24) != 0x000001; ++i) bs_get(bs, 8, "sk");
	if(i > 0) printf("[%d bytes skipped]\n", i);
}

/**================================================================================
 * @brief シーケンスヘッダを取得
 */
static int sequence_header(bitstream* bs)
{
	if(bs_get(bs, 32, "SEQH") != SEQ_HEADER_CODE)
	{
		printf("\nError: Illegal Sequence Header\n");
		return 0;
	}

	video_wd = bs_get(bs, 12, "vw");
	video_ht = bs_get(bs, 12, "vh");
	mb_wd = (video_wd + 15) >> 4;
	mb_ht = (video_ht + 15) >> 4;
	printf("# Sequence %04d\n", nseq);
	printf("Video Size: %dx%d, (%dx%d MBs)\n",
		video_wd, video_ht, mb_wd, mb_ht);
	aspect_ratio = bs_get(bs, 4, "ar");
	frame_rate_code = bs_get(bs, 4, "frc");
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
	default:
		printf("\nError: Illegal Frame Rate Code!\n");
		return 0;
	}
	printf("Frame Rate: %d (%d/%d)\n", frame_rate_code,
		frame_rate_n, frame_rate_d);
	bitrate_value = bs_get(bs, 18, "bv");
	printf("Bitrate: %d\n", bitrate_value);
	if(!bs_get(bs, 1, "m"))
	{
		printf("\nError: Illegal Marker Bit (in sequence_header)\n");
		return 0;
	}
	vbv_buf_size = bs_get(bs, 10, "vbs");
	const_param_flag = bs_get(bs, 1, "cpf");
	printf("Intra Quant Matrix: ");
	if((cust_qm_intra = bs_get(bs, 1, "iqm?")) == 1)
	{
		printf("original\n");
		for(int i = 0; i < 64; ++i) custom_qmat[1][i] = bs_get(bs, 8, "icq");
		for(int v = 0; v < 8; ++v) for(int u = 0; u < 8; ++u)
			intra_qmat[v][u] = custom_qmat[1][ZIGZAG_SCAN[0][v][u]];
	}
	else
	{
		printf("default\n");
		memcpy(intra_qmat, DEF_INTRA_QMAT, sizeof(intra_qmat));
	}
	printf("Non-Intra Quant Matrix: ");
	if((cust_qm_nonintra = bs_get(bs, 1, "nqm?")) == 1)
	{
		printf("original\n");
		for(int i = 0; i < 64; ++i) custom_qmat[0][i] = bs_get(bs, 8, "ncq");
		for(int v = 0; v < 8; ++v) for(int u = 0; u < 8; ++u)
			nonintra_qmat[v][u] = custom_qmat[0][ZIGZAG_SCAN[0][v][u]];
	}
	else
	{
		printf("default\n");
		for(int i = 0; i < 64; ++i) nonintra_qmat[0][i] = 16;
	}

	dumpx_side("# E=%d\n", nseq);
	dumpx_side("SEQ\t%d\nvw\t%d\nvh\t%d\n", nseq, video_wd, video_ht);
	dumpx_side("frc\t%d\n", frame_rate_code);
	dumpx_side("iqm?\t%d\n", cust_qm_intra);
	for(int i = 0; cust_qm_intra && i < 64; ++i)
	{
		dumpx_side(" %3d%s", custom_qmat[1][i], (i & 7) == 7 ? "\n" : "");
	}
	dumpx_side("nqm?\t%d\n", cust_qm_nonintra);
	for(int i = 0; cust_qm_nonintra && i < 64; ++i)
	{
		dumpx_side(" %3d%s", custom_qmat[0][i], (i & 7) == 7 ? "\n" : "");
	}

	dump_seq();
	return 1;
}

/**================================================================================
 * @brief シーケンス拡張を取得
 *
 * MPEG2 ではシーケンスヘッダの直後にくるはず。
 * (逆に、直後にこれが無かったら MPEG1 扱いにするのが正しい)
 */
static int sequence_extension(bitstream* bs)
{
	if(bs_get(bs, 32, "SQXH") != EXT_START_CODE || bs_get(bs, 4, "SQXI") != SEQ_EXT_ID)
	{
		printf("\nError: Illegal Sequence Extension Header\n");
		return 0;
	}

	static const char* profs[] = {"---", "High", "Spatially Scalable",
		"SNR Scalable", "Main", "Simple", "---", "---"};
	static const char* levels[] = {"---", "---", "High", "High 1440",
		"Main", "Low", "---", "---"};

	prof_and_level = bs_get(bs, 8, "pal");
	printf("Profile: %s Profile %s Level\n",
		profs[prof_and_level & 0x80 ? 0 : (prof_and_level >> 4) & 7],
		levels[(prof_and_level & 0x80 || prof_and_level & 1) ? 0
			: (prof_and_level >> 1) & 7]);
	prog_seq = bs_get(bs, 1, "ps");
	if(!prog_seq)
	{
		printf("\nError: Interlace is not supported!\n");
		return 0;
	}
	chroma_fmt = bs_get(bs, 2, "cf");
	if(bs_peek(bs, 4) != 0)
	{
		video_wd += (bs_get(bs, 2, "vwx") << 12),
		mb_wd = (video_wd + 15) >> 4;
		video_ht += (bs_get(bs, 2, "vhx") << 12),
		mb_ht = (video_ht + 15) >> 4;
		printf("Video Size (w/ext): %dx%d (%dx%d MBs)\n",
			video_wd, video_ht, mb_wd, mb_ht);
	}
	else
	{
		bs_get(bs, 2, "vwx");
		bs_get(bs, 2, "vhx");
	}
	if(bs_peek(bs, 12) != 0)
	{
		bitrate_value += (bs_get(bs, 12, "bvx") << 18);
		printf("Bitrate (w/ext): %d\n", bitrate_value);
	}
	else bs_get(bs, 12, "bvx");
	if(!bs_get(bs, 1, "m"))
	{
		printf("\nError: Illegal Marker Bit (in sequence_extension)\n");
		return 0;
	}
	vbv_buf_size += (bs_get(bs, 8, "vbsx") << 10);
	low_delay = bs_get(bs, 1, "ld");
	if(bs_peek(bs, 7) != 0)
	{
		frame_rate_n *= (bs_get(bs, 2, "frn") + 1);
		frame_rate_d *= (bs_get(bs, 5, "frd") + 1);
		printf("Frame Rate (w/ext): %d/%d\n",
			frame_rate_n, frame_rate_d);
	}
	else
	{
		bs_get(bs, 2, "frn");
		bs_get(bs, 5, "frd");
	}
	bs_align(bs, 8);

	return 1;
}

/**================================================================================
 * @brief 拡張情報やユーザーデータの取得(読み飛ばし)
 *
 * 使わない内容なので読み飛ばすだけ。
 * ただしシーケンス拡張やピクチャ拡張は別関数で読み取る。
 */
static int extension_and_user_data(bitstream* bs)
{
	uint32_t n = bs_peek(bs, 32);
	while(n == EXT_START_CODE || n == UDATA_START_CODE)
	{
		bs_get(bs, 32, "EUDH");
		next_header(bs);
	}
	return 1;
}

/**================================================================================
 * @brief GOP ヘッダの取得
 *
 * 使わない内容なので読み飛ばすだけ。
 */
static int gop_header(bitstream* bs)
{
	if(bs_get(bs, 32, "GOPH") != GROUP_START_CODE)
	{
		printf("\nError: Illegal GOP Header\n");
		return 0;
	}
	time_code = bs_get(bs, 25, "tc");
	closed_gop = bs_get(bs, 1, "cg");
	broken_link = bs_get(bs, 1, "bl");
	bs_align(bs, 8);
	return 1;
}

/**================================================================================
 * @brief ピクチャヘッダの取得
 */
static int picture_header(bitstream* bs)
{
	if(bs_get(bs, 32, "PICH") != PICTURE_START_CODE)
	{
		printf("\nError: Illegal Picture Header\n");
		return 0;
	}
	dump(dump_parser, NULL, "PC %4d # E=%d", npict, nseq);
	dump(dump_idct, NULL, "PC %4d # E=%d", npict, nseq);
	temp_ref = bs_get(bs, 10, "tr");
	pic_coding_type = bs_get(bs, 3, "pct");
	dump(dump_idct, NULL, " %d # pic_coding_type", pic_coding_type);
	dump(dump_parser, NULL, " %4d %d # temp_ref pic_coding_type", temp_ref, pic_coding_type);
	if(pic_coding_type < 1 || pic_coding_type > 2)
	{
		printf("\nError: Only I/P Frames are supported!\n");
		return 0;
	}
	// printf("Picture[%c]:", "_IP_"[pic_coding_type]);
	vbv_delay = bs_get(bs, 16, "vd");
	if(pic_coding_type & 2)
	{
		if(bs_get(bs, 1, "fpfv") != 0 ||	// full_pel_forward_vector
			bs_get(bs, 3, "ffc") != 7)		// forward_f_code
		{
			printf("\nError: MPEG1 is not supported!\n");
			return 0;
		}
	}
	if(pic_coding_type == 3)
	{
		if(bs_get(bs, 1, "fpbv") != 0 ||	// full_pel_backward_vector
			bs_get(bs, 3, "bfc") != 7)		// backward_f_code
		{
			printf("\nError: B frame is not supported!\n");
			return 0;
		}
	}
	while(bs_get(bs, 1, "ud?")) bs_get(bs, 8, "ud");
	bs_align(bs, 8);
	return 1;
}

/**================================================================================
 * @brief ピクチャ拡張の取得
 */
static int picture_coding_extension(bitstream* bs)
{
	if(bs_get(bs, 32, "PCXH") != EXT_START_CODE || bs_get(bs, 4, "pcxi") != PIC_CODE_EXT_ID)
	{
		printf("\nError: Illegal Picture Coding Extension Header");
		return 0;
	}

	f_code[0][0] = bs_get(bs, 4, "fc00");
	f_code[0][1] = bs_get(bs, 4, "fc01");
	if(bs_get(bs, 8, "fc1x") != 0xff)
	{
		printf("\nError: B Frame is not supported!\n");
		return 0;
	}
	intra_dc_precision = bs_get(bs, 2, "idp");
	// printf("(%2d)", intra_dc_precision + 8);
	picture_struct = bs_get(bs, 2, "ps");
	if(picture_struct != 0b11)
	{
		printf("\nError: Unsupported picture structure\n");
		return 0;
	}
	top_field_first = bs_get(bs, 1, "tff");
	frame_pred_frame_dct = bs_get(bs, 1, "fpfd");
	if(!frame_pred_frame_dct)
	{
		printf("\nError: frame_pred_frame_dct is zero!\n");
		return 0;
	}
	conceal_mv = bs_get(bs, 1, "cm");
	q_scale_type = bs_get(bs, 1, "qst");
	intra_vlc_fmt = bs_get(bs, 1, "ivf");
	alt_scan = bs_get(bs, 1, "as");
	if(alt_scan)
	{
		printf("\nError: Alternate Scan is not supported!\n");
		return 0;
	}
	rep_first_field = bs_get(bs, 1, "rff");
	chroma_420_type = bs_get(bs, 1, "c4t");
	prog_frame = bs_get(bs, 1, "pf");
	if(!prog_frame)
	{
		printf("\nError: Interlace is not supported!\n");
		return 0;
	}
	if(bs_get(bs, 1, "cdf"))	// composite_display_flag
	{
		bs_get(bs, 1 + 3 + 1 + 7 + 8, "cdfd");
	}
	bs_align(bs, 8);
	dump(dump_parser, NULL, "%1d %1d # dc_prec qs_type", intra_dc_precision, q_scale_type);
	dumpx_side("# E=%d,P=%d\n", nseq, npict);
	dumpx_side("PIC\t%d\nqst\t%d\nidp\t%d\nif\t%d\n", npict,
				q_scale_type, intra_dc_precision, pic_coding_type == 1 ? 1 : 0);
	return 1;
}

/**================================================================================
 * @brief ピクチャデータ(中身)のデコード
 */
static int picture_data(bitstream* bs)
{
	// バッファの切り替え
	mc_switchbuffer();

	int r = 0;

	while(1)
	{
		r = slice(bs);
		if(!r) return 0;
		if(r < 0) break;
	}

	// 出力
	if(r < 0) mc_output_yuv();

	return 1;
}

/**================================================================================
 * @brief スライスの処理
 */
static int slice(bitstream* bs)
{
	uint32_t n = bs_peek(bs, 32);
	if(n < 0x00000101 || n > 0x000001af) return -1;
	bs_get(bs, 32, "SLIH");
	slice_vert_position = (n & 0xff) - 1;
	mb_x = mb_wd - 1;
	mb_y = slice_vert_position - 1;
	if(!(q_scale_code = bs_get(bs, 5, "qsc")))
	{
		printf("\nError: Invalid q_scale_code\n");
		return 0;
	}

	while(bs_get(bs, 1, "ud?")) bs_get(bs, 8, "ud");

	if(verbose_mode)
		printf("[Slice %6d] Vert:%3d, q_scale_code:%2d\n",
			nslice, slice_vert_position, q_scale_code);

	reset_dct_dc_pred();
	mc_reset_mv_pred();
	nmb = 0;
	dump(dump_parser, NULL, "SL %4d # E=%d,P=%d", nslice, nseq, npict);
	dump(dump_idct, NULL, "SL %4d # E=%d,P=%d", nslice, nseq, npict);

	while(1)
	{
		int r = macroblock(bs);
		if(!r) return 0;
		++nmb;
		if(r < 0) break;
	}

	bs_align(bs, 8);
	return 1;
}

/**================================================================================
 * @brief マクロブロックの処理
 */
static int macroblock(bitstream* bs)
{
	int inc = 0;
	while(1)
	{
		if(!bs_peek(bs, 8)) return -1;
		int i = bs_vlc(bs, &vlc_table_b1, "mbi");
		if(i > 0)
		{
			inc += i;
			break;
		}
		inc += 33;
	}

	if(inc > 1 && pic_coding_type == 1)
	{
		printf("\nError: MB skip is prohibited in I frame!\n");
		return 0;
	}

	if(inc > 1 && pic_coding_type == 2) mc_reset_mv_pred();
	if(inc > 1) reset_dct_dc_pred();

	for(int i = 0; i < inc; ++i, ++mb_x)
	{
		if(mb_x >= mb_wd) { mb_x -= mb_wd; ++mb_y; }
		if(i == 0) continue;

		// MB スキップ対策(フレームバッファの転写)
		for(int b = 0; b < 6; ++b) CALL(mc_copybuffer(mb_x, mb_y, b));
	}

	if(mb_x >= mb_wd) { mb_x -= mb_wd; ++mb_y; }

	int mb_type;
	switch(pic_coding_type)
	{
	case 1:		// I
		mb_type = bs_vlc(bs, &vlc_table_b2, "mti"); break;
	case 2:		// P
		mb_type = bs_vlc(bs, &vlc_table_b3, "mtp"); break;
	default:
		printf("\nError: Unknown pic_coding_type!\n");
		return 0;
	}

	mb_quant = (mb_type >> 7) & 1;
	mb_mo_fw = (mb_type >> 6) & 1;	// mb_intra と同時に 1 にはならない
	mb_coded = (mb_type >> 4) & 1;
	mb_intra = (mb_type >> 3) & 1;
	if(!mb_intra) reset_dct_dc_pred();
	mb_hasmv = (mb_mo_fw || (mb_intra && conceal_mv)) ? 1 : 0;

	nblock = -1;
	dump_header(dump_mb);
	dump(dump_parser, NULL, "MB %4d # E=%d,P=%d,S=%d", nmb, nseq, npict, nslice);
	dump(dump_isdq, NULL, "MB %4d # E=%d,P=%d,S=%d", nmb, nseq, npict, nslice);
	dump(dump_idct, NULL, "MB %4d # E=%d,P=%d,S=%d", nmb, nseq, npict, nslice);
	dump(dump_mb, /*"inc "*/"mb_x mb_y", /*" %3d"*/" %2d %2d",
			/*inc, */mb_x, mb_y);
	dump(dump_parser, NULL, "%3d %3d # mb_x mb_y", mb_x, mb_y);
	dump(dump_mb, "quant mo_fw coded intra hasmv", " %d %d %d %d %d",
			mb_quant, mb_mo_fw, mb_coded, mb_intra, mb_hasmv);
	// printf("quant mo_fw coded intra hasmv = %d %d %d %d %d\n",
	// 		mb_quant, mb_mo_fw, mb_coded, mb_intra, mb_hasmv);

	// picture_struct == PIC_STRUCT_FRAME
	// frame_pred_frame_dct == 0
	// 前提のため、dct_type など省略

	if(mb_quant) mb_q_scale_code = bs_get(bs, 5, "mqsc");
	else mb_q_scale_code = q_scale_code;
	dump(dump_mb, "qs_type qs_code", " %d %2d",
			q_scale_type, mb_q_scale_code);
	dump(dump_parser, NULL, "%1d %2d # intra qs_code",
			mb_intra, mb_q_scale_code);
	dump(dump_isdq, NULL, "%1d # intra",
			mb_intra);
	dump(dump_idct, NULL, "%1d # intra",
			mb_intra);

	if(mb_hasmv)
	{
		for(int t = 0; t < 2; ++t)
		{
			mo_code[0][0][t] = bs_vlc(bs, &vlc_table_b10, t ? "mc001" : "mc000");
			if(f_code[0][t] != 1 && f_code[0][t] != 15 &&
				mo_code[0][0][t] != 0)
				mo_residual[0][0][t] = bs_get(bs, f_code[0][t] - 1, t ? "mr001" : "mr000");
			else
				mo_residual[0][0][t] = 0;
			// dmv 省略
		}

		CALL(mc_decode_mv());
	}
	else for(int t = 0; t < 2; ++t)
		mo_code[0][0][t] = mo_residual[0][0][t] = 0;

	if((mb_intra && !conceal_mv) ||
		(!mb_intra && !mb_mo_fw && pic_coding_type == 2))
		mc_reset_mv_pred();	// 少なくとも、mb_hasmv == 1 のときはここを絶対通らない

	// dump(dump_mb, "mo_code[0] mo_residual[0]", " %3d %3d",
	// 	mo_code[0][0][0], mo_residual[0][0][0]);
	// dump(dump_mb, "mo_code[1] mo_residual[1]", " %3d %3d",
	// 	mo_code[0][0][1], mo_residual[0][0][1]);
	dump(dump_mb, "mv_h mv_v", " %4d %4d", PMV[0][0][0], PMV[0][0][1]);
	dump(dump_parser, NULL, "%5d %5d # mv_h mv_v", PMV[0][0][0], PMV[0][0][1]);
	dump(dump_isdq, NULL, "%5d %5d %3d %3d # mv_h mv_v mb_x mb_y",
		PMV[0][0][0], PMV[0][0][1], mb_x, mb_y);
	dump(dump_idct, NULL, "%5d %5d %3d %3d # mv_h mv_v mb_x mb_y",
		PMV[0][0][0], PMV[0][0][1], mb_x, mb_y);

	if(mb_intra && conceal_mv && bs_get(bs, 1, "m") != 1)
	{
		printf("\nError: Illegal Marker bit (in macroblock)\n");
		return 0;
	}

	mb_pattern = mb_intra ? 0b111111 : 0;
	if(mb_coded) mb_pattern |= bs_vlc(bs, &vlc_table_b9, "mbp");

	dump(dump_mb, "pattern", " %d %d %d %d %d %d",
			(mb_pattern >> 5) & 1,
			(mb_pattern >> 4) & 1,
			(mb_pattern >> 3) & 1,
			(mb_pattern >> 2) & 1,
			(mb_pattern >> 1) & 1,
			(mb_pattern >> 0) & 1);
	dump(dump_parser, NULL, "%2d # pattern", mb_pattern);

	dumpx_side("# E=%d,P=%d,S=%d,M=%d\n", nseq, npict, nslice, nmb);
	dumpx_side("MB\t%d\nmbx\t%d\nmby\t%d\nmvh\t%d\nmvv\t%d\nmbqsc\t%d\nintra\t%d\n",
				nmb, mb_x, mb_y, PMV[0][0][0], PMV[0][0][1], mb_q_scale_code, mb_intra);

	if(verbose_mode)
	{
		printf("[MB %6d] %c%c%c%c %d%d%d%d%d%d",
			nmb, mb_quant ? 'Q' : '-', mb_mo_fw ? 'F' : '-',
			mb_coded ? 'C' : '-', mb_intra ? 'I' : '-',
			(mb_pattern >> 5) & 1,
			(mb_pattern >> 4) & 1,
			(mb_pattern >> 3) & 1,
			(mb_pattern >> 2) & 1,
			(mb_pattern >> 1) & 1,
			(mb_pattern >> 0) & 1);

		if(mb_hasmv) printf(" (%+5.1f, %+5.1f)\n",
			(double)mv[0][0][0] / 2,
			(double)mv[0][0][1] / 2);
		else printf("\n");
	}

	// 4:2:0 限定!
	for(int b = 0; b < 6; ++b)
	{
		++nblock;
		CALL(block(bs, b));
	}

	return 1;
}

/**================================================================================
 * @brief ブロックの処理
 */
static int block(bitstream* bs, int b)
{
	dump(dump_parser, NULL, "BL %1d # E=%d,P=%d,S=%d,M=%d", nblock, nseq, npict, nslice, nmb);
	dump(dump_isdq, NULL, "BL %1d # E=%d,P=%d,S=%d,M=%d", nblock, nseq, npict, nslice, nmb);
	dump(dump_idct, NULL, "BL %1d # E=%d,P=%d,S=%d,M=%d", nblock, nseq, npict, nslice, nmb);
	int pat = (mb_pattern >> (5 - b)) & 1;
	dump(dump_parser, NULL, "%1d # pat", pat);
	dump(dump_isdq, NULL, "%1d # pat", pat);
	dump(dump_idct, NULL, "%1d # pat", pat);
	char pos_info[64];
	sprintf(pos_info, "# E=%d,P=%d,S=%d,M=%d,B=%d\n", nseq, npict, nslice, nmb, nblock);
	dumpx_bitstream(pos_info);
	dumpx_side(pos_info);
	dumpx_side("BLK\t%d\n", nblock);
	dumpx_side("coded\t%d\n", pat);
	dumpx_mc_fetch(pos_info);
	dumpx_mc_mix(pos_info);
	if(pat)
	{
		dumpx_rl(pos_info);
		dumpx_isdq_out(pos_info);
		dumpx_idct_out(pos_info);
		CALL(block_coefs(bs, b));
		CALL(invscan(QFS, QF));
		CALL(dequant(QF, LF));
		CALL(idct(LF, SF));
	}
	else memset(SF, 0, sizeof(SF));

	CALL(mc(SF, b));

	dump_header(dump_mc_out);
	dump(dump_mc_out, "mb_x mb_y", " %2d %2d",
		mb_x, mb_y);

	return 1;
}

/**================================================================================
 * @brief ブロックのDCT係数(RLペア)読み込み
 */
static int block_coefs(bitstream* bs, int b)
{
	const VLC_TABLE* table_dct =
		(intra_vlc_fmt && mb_intra) ? &vlc_table_b15 : &vlc_table_b14;
	int i = 0;

	if(mb_intra)
	{
		// イントラ先頭
		int size = bs_vlc(bs, b < 4 ? &vlc_table_b12 : &vlc_table_b13, "szi1");
		int diff = 0;
		if(size > 0)
		{
			diff = bs_get(bs, size, "ifd");
			dumpx_rl("# size=%d,diff_raw=%d\n", size, diff);
			int half_range = 1 << (size - 1);
			if(diff < half_range) diff = (diff + 1) - (2 * half_range);
		}
		else dumpx_rl("# size=%d,diff_raw=%d\n", size, diff);

		int cc = (b < 4) ? 0 : (b & 1) + 1;
		QFS[i] = dct_dc_pred[cc] + diff;
		if(QFS[0] < 0 || QFS[0] >= (1 << (8 + intra_dc_precision)))
		{
			printf("\nError: QFS[0] overflow\n");
			return 0;
		}
		dct_dc_pred[cc] = QFS[i++];
	}
	else if(bs_peek(bs, 1))
	{
		// non-intra の最初の係数で、先頭が 1 の場合(特例)
		// 11 -> (0,-1)
		// 10 -> (0,+1)
		if(bs_get(bs, 2, "rlpn1") & 1)
			QFS[i++] = -1;
		else
			QFS[i++] = 1;
	}
	if(i > 0)
	{
		dumpx_rl("%d\t%d\n", 0, QFS[0]);
		dump(dump_parser, NULL, "%2d 0 %5d", 0, QFS[0]);
	}

	// 残りの係数
	while(1)
	{
		int c = bs_vlc(bs, table_dct, "rlp");
		switch(bs_vlclen(bs))
		{
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			cycle_esti += 4;
			break;
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
			cycle_esti += 5;
			break;
		default:
			cycle_esti += 6;
		}
		int run, level;
		const char* s = NULL;
		if(c == -1) break;	// EOB
		if(c == -9)
		{
			// Escape
			run = bs_get(bs, 6, "escr");
			level = bs_get(bs, 12, "escl");
			if(level >= 2048) level -= 4096;
			if(level == -2048 || level == 0)
			{
				printf("\nError: Detected forbidden level value\n");
				return 0;
			}
			s = "# escape";
		}
		else if(bs_get(bs, 1, "rls"))
		{
			// level is negative
			run = c >> 8; level = -(c & 255);
		}
		else
		{
			// level is positive
			run = c >> 8; level = (c & 255);
		}

		if((i + run) >= 64)
		{
			printf("\nError: Number of RL pair is too large!\n");
			return 0;
		}

		dumpx_rl("%d\t%d\n", run, level);
		dump(dump_parser, NULL, "%2d %d %5d%s", run, s ? 1 : 0, level, s ? " # escape" : "");
		for(; run > 0; ++i, --run) QFS[i] = 0;
		QFS[i++] = level;
	}

	for(; i < 64; ++i) QFS[i] = 0;		// 最後まで 0 で埋める

	dumpx_rl("0\t0\t# eob\n");
	dump(dump_parser, NULL, "%2d 0 %5d # EOB", 0, 0);

	return 1;
}

/**================================================================================
 * @brief DCT 係数の DC 成分予測のリセット
 */
static void reset_dct_dc_pred()
{
	int i = (1 << (intra_dc_precision + 7));
	dct_dc_pred[0] = dct_dc_pred[1] = dct_dc_pred[2] = i;
}

