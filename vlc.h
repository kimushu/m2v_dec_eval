//================================================================================
// m2v_dec_eval - VLC テーブル
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
extern const VLC_ENTRY vlc_table_b2[];
extern const VLC_ENTRY vlc_table_b3[];
extern const VLC_ENTRY vlc_table_b4[];
extern const VLC_ENTRY vlc_table_b9[];
extern const VLC_ENTRY vlc_table_b10[];
extern const VLC_ENTRY vlc_table_b11[];
extern const VLC_ENTRY vlc_table_b12[];
extern const VLC_ENTRY vlc_table_b13[];
extern const VLC_ENTRY vlc_table_b14[];
extern const VLC_ENTRY vlc_table_b15[];


#endif	/* !_VLC_H_ */

