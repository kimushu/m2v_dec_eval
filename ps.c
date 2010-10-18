//================================================================================
// m1v_dec_eval - PS デコーダ
// ref: ISO13818-1
// $Id$
//================================================================================

#include "ps.h"

static int pack_header();
static int pes_packet();

int decode_pack()
{
	// Table 2-32
	pack_header();
	while(1)	// TODO:終了条件
	{
	}
}

static int pack_header()
{
}

