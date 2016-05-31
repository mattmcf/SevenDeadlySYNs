#include "ChunkyFile.h"
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <wordexp.h>
#include <sys/types.h>
#include <utime.h>
#include <string.h>
#include <assert.h>

typedef struct
{
	char* path;
	char* data;
	int size;
	char* chunks_written;
	time_t modify_time;
} _ChunkyFile;

ChunkyFile* chunkyfile_new_for_reading_from_path(char* path)
{
    struct stat st;
    stat(path, &st);
	
	if (!S_ISREG(st.st_mode))
	{
		return NULL;
	}
	
	_ChunkyFile* cf = (_ChunkyFile*)malloc(sizeof(_ChunkyFile));
		
	cf->path = NULL;
	cf->size = st.st_size;
	cf->data = (char*)malloc(st.st_size * sizeof(char));
	cf->chunks_written = (char*)calloc(num_chunks_for_size(st.st_size), sizeof(char));
	FILE* file = fopen(path, "r");
	for (int i = 0; i < cf->size; i++)
	{
		cf->data[i] = fgetc(file);
	}
	fclose(file);
	
	return (ChunkyFile*)cf;
}

ChunkyFile* chunkyfile_new_for_writing_to_path(char* path, int size, time_t modify_time)
{
	_ChunkyFile* cf = (_ChunkyFile*)malloc(sizeof(_ChunkyFile));
	
    //wordexp_t exp_result;
    //wordexp(path, &exp_result, 0);
	
	cf->path = strdup(path); //exp_result.we_wordv[0]);
	cf->data = (char*)malloc(size * sizeof(char));
	cf->chunks_written = (char*)calloc(num_chunks_for_size(size), sizeof(char));
	cf->size = size;
	cf->modify_time = modify_time;
	
	char* db = "DEADBEEF";
	for (int i = 0; i < size; i++)
	{
		cf->data[i] = db[i % 8];
	}
	
	return (ChunkyFile*)cf;
}

void chunkyfile_write(ChunkyFile* chunkyfile)
{
	_ChunkyFile* cf = (_ChunkyFile*)chunkyfile;
	
	assert(cf->path);
	
	FILE* file = fopen(cf->path, "w+");
	
	for (int i = 0; i < cf->size; i++)
	{
		fputc(cf->data[i], file);
	}
	
	fclose(file);
	
    struct utimbuf tbuf;
	tbuf.actime = cf->modify_time;
	tbuf.modtime = cf->modify_time;
	
	utime(cf->path, &tbuf);
}

int num_chunks_for_size(int size)
{
	if (size % CHUNKYFILE_CHUNK_SIZE == 0)
	{
		return size / CHUNKYFILE_CHUNK_SIZE;
	}
	return size / CHUNKYFILE_CHUNK_SIZE + 1;
}

int chunkyfile_num_chunks(ChunkyFile* chunkyfile)
{
	_ChunkyFile* cf = (_ChunkyFile*)chunkyfile;
	
	return num_chunks_for_size(cf->size);
}

void chunkyfile_get_chunk(ChunkyFile* chunkyfile, int chunkNum, char** chunk, int* chunkSize)
{
	_ChunkyFile* cf = (_ChunkyFile*)chunkyfile;
	
	*chunk = cf->data + chunkNum * CHUNKYFILE_CHUNK_SIZE;
	
	if (chunkNum == chunkyfile_num_chunks(chunkyfile) - 1)
	{
		*chunkSize = cf->size % CHUNKYFILE_CHUNK_SIZE;
	}
	else
	{
		*chunkSize = CHUNKYFILE_CHUNK_SIZE;
	}
}

void chunkyfile_set_chunk(ChunkyFile* chunkyfile, int chunkNum, char*  chunk, int  chunkSize)
{
	_ChunkyFile* cf = (_ChunkyFile*)chunkyfile;
		
	if (chunkNum == chunkyfile_num_chunks(chunkyfile) - 1)
	{
		if (chunkSize != cf->size % CHUNKYFILE_CHUNK_SIZE)
		{
			printf("Error. Incorrect chunksize specified.\n");
		}
	}
	else
	{
		if (chunkSize != CHUNKYFILE_CHUNK_SIZE)
		{
			printf("Error. Incorrect chunksize specified.\n");
		}
	}
	
	cf->chunks_written[chunkNum] = 1;
	
	memcpy(cf->data + chunkNum * CHUNKYFILE_CHUNK_SIZE, chunk, chunkSize);
}

int chunkyfile_all_chunks_written(ChunkyFile* chunkyfile)
{
	_ChunkyFile* cf = (_ChunkyFile*)chunkyfile;
	
	for (int i = 0; i < chunkyfile_num_chunks(chunkyfile); i++)
	{
		if (cf->chunks_written[i] == 0)
		{
			return 0;
		}
	}
	return 1;
}

void chunkyfile_destroy(ChunkyFile* chunkyfile)
{
	_ChunkyFile* cf = (_ChunkyFile*)chunkyfile;
	
	if (cf->path)
	{
		free(cf->path);
	}
	free(cf->chunks_written);
	free(cf->data);
	free(cf);
}

/*
int main()
{
	ChunkyFile* cf = chunkyfile_new_for_reading_from_path("/Users/jacob/Cantor Deitell\'s Best - Cantor Paul Deitell - Mi Sheoso Nisim.mp3");
	
	if (cf == NULL)
	{
		printf("cf is null\n");
		return 0;
	}
	
	int nc = chunkyfile_num_chunks(cf);
	
	for (int i = 0; i < nc; i++)
	{
		char* chunk;
		int length;
		chunkyfile_get_chunk(cf, i, &chunk, &length);
		for (int j = 0; j < length; j++)
		{
			putchar(chunk[j]);
		}
	}
	
	printf("Num Chunks: %d\n", chunkyfile_num_chunks(cf));
	
	chunkyfile_write(cf);
	
	return 0;
}
*/











