#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "../Queue/Queue.h"

typedef struct FileSystem FileSystem;
typedef struct Folder Folder;
typedef struct File File;
typedef struct FileSystemIterator FileSystemIterator;

FileSystem* filesystem_new(char* path);
void filesystem_print(FileSystem* fs);
void filesystem_diff(FileSystem* filesystem, FileSystem** additions, FileSystem** deletions);
FileSystem* filesystem_copy(FileSystem* filesystem);
char* filesystem_get_root_path(FileSystem* filesystem);
void filesystem_destroy(FileSystem* fs);

FileSystemIterator* filesystemiterator_new(FileSystem* fs);
char* filesystemiterator_next(FileSystemIterator* iterator);
void filesystemiterator_destroy(FileSystemIterator* iterator);



#endif