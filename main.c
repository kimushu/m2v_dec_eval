//================================================================================
// m1v_dec_eval - メイン
// $Id$
//================================================================================

#include <stdio.h>
#include <string.h>
#include "m1vdec.h"
#include "ps.h"

static const char* process_ps(const char* file);
static const char* process_video(const char* file);

FILE* fpout_v;
FILE* fpout_a;

int main(int argc, char* argv[])
{
	const char* r = NULL;
	if(argc != 3) r = "invalid option";
	if(!r)
	{
		if(!strcmp(argv[1], "-p"))
			r = process_ps(argv[2]);
		else if(!strcmp(argv[1], "-v"))
			r = process_video(argv[2]);
		else
			r = "unknown option";
	}

	if(r)
	{
		fprintf(stderr, "%s\n", r);
		return 1;
	}

	return 0;
}

static const char* process_ps(const char* file)
{
	char fn[256];
	strcpy(fn, file);
	strcat(fn, ".vs");
	fpout_v = fopen(fn, "wb");
	strcpy(fn, file);
	strcat(fn, ".as");
	fpout_a = fopen(fn, "wb");
	CALL(bs_open(file));
	CALL(decode_pack());
	fclose(fpout_v);
	fclose(fpout_a);
	return NULL;
}

static const char* process_video(const char* file)
{
	CALL(bs_open(file));
	CALL(decode_video());
	return NULL;
}

