#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "../Queue/Queue.h"
#include <assert.h>
#include "ColoredPrint.h"

char* FORMATS[] =
{
	"\x1b[0m",
	"\x1b[1m",
	"\x1b[2m",
	"\x1b[4m",
	"\x1b[5m",
	"\x1b[7m",

	"\x1b[30m",
	"\x1b[31m",
	"\x1b[32m",
	"\x1b[33m",
	"\x1b[34m",
	"\x1b[35m",
	"\x1b[36m",
	"\x1b[37m",

	"\x1b[40m",
	"\x1b[41m",
	"\x1b[42m",
	"\x1b[43m",
	"\x1b[44m",
	"\x1b[45m",
	"\x1b[46m",
	"\x1b[47m",

	"\x1b[90m",
	"\x1b[91m",
	"\x1b[92m",
	"\x1b[93m",
	"\x1b[94m",
	"\x1b[95m",
	"\x1b[96m",
	"\x1b[97m",


	"\x1b[100m",
	"\x1b[101m",
	"\x1b[102m",
	"\x1b[103m",
	"\x1b[104m",
	"\x1b[105m",
	"\x1b[106m",
	"\x1b[107m"
};

Queue* get_format_queue()
{
	static Queue* q = NULL;
	
	if (!q)
	{
		q = queue_new();
	}
	
	return q;
}

int register_format(FORMAT_ARG formats[])
{
	Queue* q = get_format_queue();
	int id = queue_length(q);
	
	long len = sizeof(FORMATS) / sizeof(FORMATS[0]);
	FORMAT_ARG* f = (FORMAT_ARG*)malloc(len * sizeof(FORMAT_ARG));
	
	for (int i = 0; i < 40; i++)
	{
		f[i] = formats[i];
		if (formats[i] == 0)
		{
			break;
		}
	}
	
	queue_push(q, f);
	return id;
}

void format_printf(int formatID, char* format, ...)
{
	Queue* q = get_format_queue();
	
	assert(formatID < queue_length(q) && formatID >= 0);

	FORMAT_ARG* formats = queue_get(q, formatID);
	
	for (int i = 0; ; i++)
	{
		if (formats[i] == 0)
		{
			break;
		}
		printf("%s", FORMATS[formats[i]]);
	}

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
	
	printf("%s", FORMATS[COLOR_RESET]);
}

/*
int main (int argc, char const *argv[]) 
{
	FORMAT_ARG format[] = {COLOR_FLASH, COLOR_BOLD, COLOR_UNDERLINE, COLOR_BG_L_RED, 0};
	int id = register_format(format);
	
	format_printf(id, "Hello!\n");

  return 0;
}
*/
