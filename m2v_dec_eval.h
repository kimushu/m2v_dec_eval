//================================================================================
// m2v_dec_eval - MPEG2 デコーダ評価用コード
// ref: ISO13818-2
// $Id$
//================================================================================

#ifndef _M2V_DEV_EVAL_H_
#define _M2V_DEV_EVAL_H_


// global variables (prototypes)
int video_wd, video_ht, mb_wd, mb_ht,
	aspect_ratio, frame_rate_code, frame_rate_n, frame_rate_d,
	bitrate_value, vbv_buf_size, const_param_flag;
int intra_qmat[8][8], nonintra_qmat[8][8];
int prof_and_level, prog_seq, chroma_fmt, low_delay;
int time_code, closed_gop, broken_link;
int temp_ref, pic_coding_type, vbv_delay;
int f_code[1][2], intra_dc_precision, picture_struct, top_field_first,
	frame_pred_frame_dct, conceal_mv, q_scale_type, intra_vlc_fmt,
	alt_scan, rep_first_field, chroma_420_type, prog_frame;

#endif	/* !_M2V_DEV_EVAL_H_ */

