#ifndef CHUNKYFILE_H
#define CHUNKYFILE_H

#include <sys/types.h>

#define CHUNKYFILE_CHUNK_SIZE (1024)

typedef struct ChunkyFile ChunkyFile;

ChunkyFile* chunkyfile_new_for_reading_from_path(char* path);
ChunkyFile* chunkyfile_new_for_writing_to_path(char* path, int size, time_t modify_time);

void chunkyfile_write(ChunkyFile* chunkyfile);

int chunkyfile_num_chunks(ChunkyFile* chunkyfile);

void chunkyfile_get_chunk(ChunkyFile* chunkyfile, int chunkNum, char** chunk, int* chunkSize);
void chunkyfile_set_chunk(ChunkyFile* chunkyfile, int chunkNum, char*  chunk, int  chunkSize);

void chunkyfile_destroy(ChunkyFile* chunkyfile);

int num_chunks_for_size(int size);

int chunkyfile_all_chunks_written(ChunkyFile* chunkyfile);

#endif
