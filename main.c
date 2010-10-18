//================================================================================
// m1v_dec_eval - メイン
// $Id$
//================================================================================

#include <stdio.h>
#include "ps.h"

int main(int argc, char* argv[])
{
	const char* r;
	r = bs_open("/home/kimura/gs_noaud.mpg");
	if(r)
	{
		fprintf(stderr, "%s\n", r);
		return 1;
	}
	r = decode_pack();
	if(r)
	{
		fprintf(stderr, "%s\n", r);
		return 1;
	}
	return 0;
}

