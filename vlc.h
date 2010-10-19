//================================================================================
// m1v_dec_eval - VLC テーブル
// $Id$
//================================================================================

#ifndef _VLC_H_
#define _VLC_H_

#include <stdint.h>

// types
typedef struct VLC_ENTRY
{
	uint32_t code;
	int clen;
	int value;
} VLC_ENTRY;

// exports
extern const VLC_ENTRY vlc_table_b1[];
extern const VLC_ENTRY vlc_table_b2a[];
extern const VLC_ENTRY vlc_table_b2b[];
extern const VLC_ENTRY vlc_table_b2c[];
extern const VLC_ENTRY vlc_table_b2d[];
extern const VLC_ENTRY vlc_table_b3[];
extern const VLC_ENTRY vlc_table_b4[];
extern const VLC_ENTRY vlc_table_b5a[];
extern const VLC_ENTRY vlc_table_b5b[];
extern const VLC_ENTRY vlc_table_dct_f[];
extern const VLC_ENTRY vlc_table_dct_n[];


#endif	/* !_VLC_H_ */

