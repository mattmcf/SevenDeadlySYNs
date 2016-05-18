#include "ChunkyFile.h"
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct
{
	char* data;
	int size;
} _ChunkyFile;

ChunkyFile* chunkyfile_new_from_path(char* path)
{
    struct stat st;
    stat(path, &st);
	
	if (!S_ISREG(st.st_mode))
	{
		return NULL;
	}
	
	_ChunkyFile* cf = (_ChunkyFile*)malloc(sizeof(_ChunkyFile));
		
	cf->size = st.st_size;
	cf->data = (char*)malloc(st.st_size * sizeof(char));

	FILE* file = fopen(path, "r");
	for (int i = 0; i < cf->size; i++)
	{
		cf->data[i] = fgetc(file);
	}
	fclose(file);
	
	return (ChunkyFile*)cf;
}

ChunkyFile* chunkyfile_new_empty(int size)
{
	_ChunkyFile* cf = (_ChunkyFile*)malloc(sizeof(_ChunkyFile));
	cf->data = (char*)malloc(size * sizeof(char));
	cf->size = size;
	return (ChunkyFile*)cf;
}

void chunkyfile_write_to_path(ChunkyFile* chunkyfile, char* path)
{
	FILE* file = fopen(path, "w+");
	_ChunkyFile* cf = (_ChunkyFile*)chunkyfile;
	
	for (int i = 0; i < cf->size; i++)
	{
		fputc(cf->data[i], file);
	}
	
	fclose(file);
}

int chunkyfile_num_chunks(ChunkyFile* chunkyfile)
{
	_ChunkyFile* cf = (_ChunkyFile*)chunkyfile;
	
	if (cf->size % CHUNKYFILE_CHUNK_SIZE == 0)
	{
		return cf->size / CHUNKYFILE_CHUNK_SIZE;
	}
	return cf->size / CHUNKYFILE_CHUNK_SIZE + 1;
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
	
	memcpy(cf->data + chunkNum * CHUNKYFILE_CHUNK_SIZE, chunk, chunkSize);
}



