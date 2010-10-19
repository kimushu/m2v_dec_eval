//================================================================================
// m1v_dec_eval - データダンプ
// $Id$
//================================================================================

#ifndef _DUMP_H_
#define _DUMP_H_

#include <stdio.h>

// exports
extern const char* dump_start(const char* ref_dir);
extern void dump_finish();
extern void dump(FILE* fp, const char* desc, const char* fmt, ...);
extern FILE* dump_slice;
extern FILE* dump_mb;


#endif	/* !_DUMP_H_ */

