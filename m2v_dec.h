//================================================================================
// m2v_dec - MPEG2 デコーダ評価用コード
// ref: ISO13818-2
// $Id: m2v_dec.h 98 2012-04-07 03:52:11Z kimu_shu $
//================================================================================

#ifndef _M2V_DEC_H_
#define _M2V_DEC_H_

#include "bitreader.h"

// global variables (prototypes)

// video.c
extern int verbose_mode;
extern int nseq, npict, nslice, nmb, nblock;
extern int start_pict, start_slice, end_pict, end_slice;
extern int video_wd, video_ht, mb_wd, mb_ht, cust_qm_intra, cust_qm_nonintra,
			aspect_ratio, frame_rate_code, frame_rate_n, frame_rate_d,
			bitrate_value, vbv_buf_size, const_param_flag;
extern int prof_and_level, prog_seq, chroma_fmt, low_delay;
extern int time_code, closed_gop, broken_link;
extern int temp_ref, pic_coding_type, vbv_delay;
extern int f_code[1][2], intra_dc_precision, picture_struct, top_field_first,
			frame_pred_frame_dct, conceal_mv, q_scale_type, intra_vlc_fmt,
			alt_scan, rep_first_field, chroma_420_type, prog_frame;
extern int slice_vert_position, mb_x, mb_y, q_scale_code;
extern int dct_dc_pred[3];
extern int dct_dc_pred[3], mb_quant, mb_mo_fw, mb_coded, mb_intra, mb_hasmv,
			mb_q_scale_code, mb_pattern;
extern int cycle_esti;
extern int QFS[64], QF[8][8], LF[8][8], SF[8][8], SP[8][8], SD[8][8];
extern char input[];

// mc.c
extern int mo_code[1][1][2], mo_residual[1][1][2], mv[1][1][2], PMV[1][1][2];

// dequant.c
extern int intra_qmat[8][8], nonintra_qmat[8][8];
extern int custom_qmat[2][64];
extern const int ZIGZAG_SCAN[2][8][8];
extern const int DEF_INTRA_QMAT[8][8];

// function prototypes
extern void dump_header(FILE* fp);
extern int bitdecode(bitstream* bs);
extern int invscan(int qfs[64], int qf[8][8]);
extern int dequant(int qf[8][8], int lf[8][8]);
extern int idct(int lf[8][8], int sf[8][8]);
extern int mc(int SF[8][8], int b);
extern void mc_reset_mv_pred();
extern int mc_decode_mv();
extern int mc_allocbuffer();
extern void mc_freebuffer();
extern void mc_switchbuffer();
extern int mc_copybuffer(int mbx, int mby, int b);
extern void mc_output_yuv();
extern int mc_memwrite(int x, int y, int cc, int value);


#endif	/* !_M2V_DEC_H_ */

