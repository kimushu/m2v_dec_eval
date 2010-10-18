//================================================================================
// m1v_dec_eval - ビットストリーム
// $Id$
//================================================================================

#ifndef _BITSTREAM_H_
#define _BITSTREAM_H_

#include <stdint.h>

// exports
extern const char* bs_open(const char* file);
extern uint32_t bs_get(int len);
extern uint8_t bs_gets(int len);
extern uint32_t bs_peek(int len);


#endif	/* !_BITSTREAM_H_ */

