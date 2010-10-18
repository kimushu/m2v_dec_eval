//================================================================================
// m1v_dec_eval - PS デコーダ
// ref: ISO13818-1
// $Id$
//================================================================================

#include "ps.h"

static const char* pack_header();
static const char* pes_packet();

const char* decode_pack()
{
	// Table 2-32
	const char* r = pack_header();
	if(r) return r;
	while(1)	// TODO:終了条件
	{
		r = pes_packet();
		if(r) return r;
	}
	return 0;
}

static const char* pack_header()
{
	if(bs_get(32) != 0x000001ba) return "illegal pack_start_code";
	if(bs_gets(2) != 1) return "illegal pack_header '01'";
	return 0;
}

static const char* pes_packet()
{
	return 0;
}

