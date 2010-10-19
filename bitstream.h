//================================================================================
// m1v_dec_eval - ビットストリーム
// $Id$
//================================================================================

#ifndef _BITSTREAM_H_
#define _BITSTREAM_H_

#include <stdint.h>
#include "vlc.h"

// exports
extern int g_total_bits;
extern const char* bs_open(const char* file);
extern uint32_t bs_get(int len);
extern uint8_t bs_gets(int len);
extern uint32_t bs_peek(int len);
extern uint32_t bs_align(int bits);
extern uint16_t bs_nextcode(uint32_t code, int bytes);
extern int bs_vlc(const VLC_ENTRY* table);


#endif	/* !_BITSTREAM_H_ */

