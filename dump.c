//================================================================================
// m1v_dec_eval - データダンプ
// $Id$
//================================================================================

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "dump.h"

// file pointers
FILE *dump_slice, *dump_mb, *dump_block, *dump_dequant;
int dump_enable;

#define DUMP_OPEN(x)	({ strcpy(fn, #x ".txt"); if(!(dump_##x = fopen(path, "w"))) \
							return "cannot open ref_*/" #x ".txt"; })
#define DUMP_CLOSE(x)	({ if(dump_##x) fclose(dump_##x); })

const char* dump_init(const char* ref_dir)
{
	dump_enable = 0;
	if(!ref_dir) return NULL;

	char path[256];
	strcpy(path, ref_dir);
	char* fn = path + strlen(path);

	DUMP_OPEN(slice);
	DUMP_OPEN(mb);
	DUMP_OPEN(block);
	DUMP_OPEN(dequant);

	return NULL;
}

void dump_start()
{
	dump_enable = 1;
}

void dump_finish()
{
	DUMP_CLOSE(slice);
	DUMP_CLOSE(mb);
	DUMP_CLOSE(block);
	DUMP_CLOSE(dequant);
}

void dump(FILE* fp, const char* desc, const char* fmt, ...)
{
	if(!fp || !dump_enable) return;

	va_list args;
	va_start(args, fmt);

	char buf[256];
	vsprintf(buf, fmt, args);

	if(desc && *desc)
		fprintf(fp, "%-*s # %s\n", 39, buf, desc);
	else
		fprintf(fp, "%s\n", buf);

	va_end(args);
}

