//================================================================================
// m1v_dec_eval - PS ¥Ç¥³¡¼¥À
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
	while(1)	// TODO:½ªÎ»¾ò·ï
	{
	}
}

static int pack_header()
{
}

