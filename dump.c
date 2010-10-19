//================================================================================
// m1v_dec_eval - データダンプ
// $Id$
//================================================================================

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "dump.h"

// file pointers
FILE *dump_slice, *dump_mb;

#define DUMP_OPEN(x)	({ strcpy(fn, #x ".txt"); if(!(dump_##x = fopen(path, "w"))) \
							return "cannot open ref_*/" #x ".txt"; })
#define DUMP_CLOSE(x)	({ if(dump_##x) fclose(dump_##x); })

const char* dump_start(const char* ref_dir)
{
	if(!ref_dir) return NULL;

	char path[256];
	strcpy(path, ref_dir);
	char* fn = path + strlen(path);

	DUMP_OPEN(slice);
	DUMP_OPEN(mb);

	return NULL;
}

void dump_finish()
{
	DUMP_CLOSE(slice);
	DUMP_CLOSE(mb);
}

void dump(FILE* fp, const char* desc, const char* fmt, ...)
{
	if(!fp) return;

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

