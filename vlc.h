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

typedef struct VLC_TABLE
{
	int bits;
	int* table;
} VLC_TABLE;

// exports
extern VLC_TABLE vlc_table_b1;
extern VLC_TABLE vlc_table_b2;
extern VLC_TABLE vlc_table_b3;
extern VLC_TABLE vlc_table_b4;
extern VLC_TABLE vlc_table_b9;
extern VLC_TABLE vlc_table_b10;
extern VLC_TABLE vlc_table_b11;
extern VLC_TABLE vlc_table_b12;
extern VLC_TABLE vlc_table_b13;
extern VLC_TABLE vlc_table_b14;
extern VLC_TABLE vlc_table_b15;

// function prototypes
int vlc_init_table();
void vlc_free();


#endif	/* !_VLC_H_ */

