//================================================================================
// m2v_dec_eval - MPEG2 ビデオ デコーダ
// ref: ISO13818-2
// $Id: video.h 26 2010-10-25 08:21:26Z aberi $
//================================================================================

#ifndef _VIDEO_H_
#define _VIDEO_H_

// exports
const char* decode_video(const char* ref_dir, int slices, int skips);

#endif	/* !_VIDEO_H_ */

