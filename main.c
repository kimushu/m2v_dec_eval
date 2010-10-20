//================================================================================
// m1v_dec_eval - メイン
// $Id$
//================================================================================

#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include "m1vdec.h"
#include "ps.h"
#include "video.h"
#include "bitstream.h"

static const char* process_ps(const char* file);
static const char* process_video(const char* file, int ref_out, int slices, int skips);

FILE* fpout_v;
FILE* fpout_a;

int main(int argc, char* argv[])
{
	const char* r = NULL;
	const char *ps = NULL, *video = NULL;
	int ref_out = 0;
	int slices = 0;
	int skips = 0;

	int ch;
	while((ch = getopt(argc, argv, "p:v:s:k:r")) != -1)
	{
		switch(ch)
		{
		case 'p': ps = optarg; break;
		case 'v': video = optarg; break;
		case 's': slices = atoi(optarg); break;
		case 'k': skips = atoi(optarg); break;
		case 'r': ref_out = 1; break;
		default:
			fprintf(stderr, "unknown option: %c\n", ch);
			return 1;
		}
	}
	if(optind < argc)
	{
		fprintf(stderr, "unknown option: %s\n", argv[optind]);
		return 1;
	}

	if((ps && video) || (!ps && !video))
		r = "one of ps or video needed";

	if(!r)
	{
		if(ps) r = process_ps(ps);
		if(video) r = process_video(video, ref_out, slices, skips);
	}

	if(r)
	{
		fprintf(stderr, "%s\nposition: %d bytes (+%d bits)\n",
			r, g_total_bits / 8, g_total_bits % 8);
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

static const char* process_video(const char* file, int ref_out, int slices, int skips)
{
	char refdir[256];
	if(ref_out)
	{
		strcpy(refdir, file);
		char* sl = strrchr(refdir, '/');
		if(!sl) sl = refdir;
		else sl++;
		strcpy(sl, "ref_");
		strcat(sl, file + (sl - refdir));
		if(mkdir(refdir, 0777) && errno != EEXIST)
		{
			fprintf(stderr, "cannot mkdir %s\n", refdir);
			return "mkdir failed";
		}
		strcat(sl, "/");
	}

	CALL(bs_open(file));
	CALL(decode_video(ref_out ? refdir : NULL, slices, skips));
	return NULL;
}

