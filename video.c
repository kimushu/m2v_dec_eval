//================================================================================
// m2v_dec_eval - MPEG2 ビデオ デコーダ
// ref: ISO13818-2
// $Id: video.c 98 2012-04-07 03:52:11Z kimu_shu $
//================================================================================

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <limits.h>
#include "m2v_dec.h"
#include "ps.h"
#include "dump.h"

// global variables
int verbose_mode;
int nseq, npict, nslice, nmb, nblock;
		// nmbはスライス毎に0リセット,nblockはMB毎に(ry
int start_pict, start_slice, end_pict, end_slice;
int video_wd, video_ht, mb_wd, mb_ht, cust_qm_intra, cust_qm_nonintra,
	aspect_ratio, frame_rate_code, frame_rate_n, frame_rate_d,
	bitrate_value, vbv_buf_size, const_param_flag;
int prof_and_level, prog_seq, chroma_fmt, low_delay;
int time_code, closed_gop, broken_link;
int temp_ref, pic_coding_type, vbv_delay;
int f_code[1][2], intra_dc_precision, picture_struct, top_field_first,
	frame_pred_frame_dct, conceal_mv, q_scale_type, intra_vlc_fmt,
	alt_scan, rep_first_field, chroma_420_type, prog_frame;
int slice_vert_position, mb_x, mb_y, q_scale_code;
int dct_dc_pred[3], mb_quant, mb_mo_fw, mb_coded, mb_intra, mb_hasmv,
	mb_q_scale_code, mb_pattern;
int cycle_esti;
int QFS[64], QF[8][8], LF[8][8], SF[8][8], SP[8][8], SD[8][8];
char input[PATH_MAX];

////////////////////////////////////////////////////////////////////////////////
/// @brief エントリポイント
///
int main(int argc, char* argv[])
{
	int dump = 0;
	input[0] = 0;
	start_pict = start_slice = end_pict = end_slice = 0;
	int ch;
	while((ch = getopt(argc, argv, "p:i:ds:S:t:T:v")) != -1) switch(ch)
	{
	case 'p':
		// PS の分解
		if(!decode_pack(optarg)) return 1;
		return 0;
	case 'i': strcpy(input, optarg); break;
	case 's': start_slice = atoi(optarg); break;
	case 'S': start_pict = atoi(optarg); break;
	case 't': end_slice = atoi(optarg); break;
	case 'T': end_pict = atoi(optarg); break;
	case 'd': dump = 1; break;
	case 'v': verbose_mode = 1; break;
	default:
		printf("Error: Unknown option: %c\n", ch);
		return 1;
	}
	if(!input[0])
	{
		printf(
		"Usage:\n"
		"  (PS Extraction)  %s -p <mpgfile>\n"
		"  (Video Decode)   %s -i <input> [options]\n"
		"\n"
		"Options:\n"
		"  -v          Verbose output\n"
		"  -d          Enable dump\n"
		"  -s <num>    Specify dump-start slice\n"
		"  -S <num>    Specify dump-start picture\n"
		"  -t <num>    Specify duration in slices\n"
		"  -T <num>    Specify duration in pictures\n"
		"\n"
		, argv[0], argv[0]);
		return 1;
	}

	end_slice += start_slice;
	end_pict += start_pict;

	if(dump)
	{
		char dump_dir[PATH_MAX];
		strcpy(dump_dir, input);
		char* sl = strrchr(dump_dir, '/');
		if(!sl) sl = dump_dir - 1;
		strcpy(sl + 1, "ref_");
		strcpy(sl + 5, input + (sl - dump_dir + 1));
		mkdir(dump_dir, 0777);
		if(!dump_init(dump_dir)) return 0;
	}

	bitstream* bs = bs_open(input);
	if(!bs) return 1;
	int r = bitdecode(bs);

	bs_close(bs);

	return r ? 0 : 1;
}


