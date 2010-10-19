//================================================================================
// m1v_dec_eval - 共通ヘッダ
// $Id$
//================================================================================

#ifndef _M1VDEC_H_
#define _M1VDEC_H_

#include <stdio.h>

// macros
#define CALL(x)		({ const char* r = x; if(r) return r; })

// exports
extern FILE* fpout_v;
extern FILE* fpout_a;


#endif	/* !_M1VDEC_H_ */


