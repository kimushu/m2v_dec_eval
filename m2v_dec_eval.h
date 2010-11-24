//================================================================================
// m2v_dec_eval - MPEG2 デコーダ評価用コード
// ref: ISO13818-2
// $Id$
//================================================================================

#ifndef _M2V_DEV_EVAL_H_
#define _M2V_DEV_EVAL_H_

#include "bitreader.h"

// global variables (prototypes)
extern int video_wd, video_ht, mb_wd, mb_ht,
			aspect_ratio, frame_rate_code, frame_rate_n, frame_rate_d,
			bitrate_value, vbv_buf_size, const_param_flag;
extern int intra_qmat[8][8], nonintra_qmat[8][8];
extern int prof_and_level, prog_seq, chroma_fmt, low_delay;
extern int time_code, closed_gop, broken_link;
extern int temp_ref, pic_coding_type, vbv_delay;
extern int f_code[1][2], intra_dc_precision, picture_struct, top_field_first,
			frame_pred_frame_dct, conceal_mv, q_scale_type, intra_vlc_fmt,
			alt_scan, rep_first_field, chroma_420_type, prog_frame;


// function prototypes
extern int bitdecode(bitstream* bs);
extern int mc_allocbuffer(int mb_wd, int mb_ht);
extern void mc_switchbuffer();


#endif	/* !_M2V_DEV_EVAL_H_ */

