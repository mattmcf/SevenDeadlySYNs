#ifndef CHUNKYFILE_H
#define CHUNKYFILE_H

#define CHUNKYFILE_CHUNK_SIZE (1024)

typedef struct ChunkyFile ChunkyFile;

ChunkyFile* chunkyfile_new_from_path(char* path);
ChunkyFile* chunkyfile_new_empty(int size);

void chunkyfile_write_to_path(ChunkyFile* chunkyfile, char* path);

int chunkyfile_num_chunks(ChunkyFile* chunkyfile);

void chunkyfile_get_chunk(ChunkyFile* chunkyfile, int chunkNum, char** chunk, int* chunkSize);
void chunkyfile_set_chunk(ChunkyFile* chunkyfile, int chunkNum, char*  chunk, int  chunkSize);

#endif
