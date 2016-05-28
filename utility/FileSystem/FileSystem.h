#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "../Queue/Queue.h"

char* copy_string(char* string);
void push_string(Queue* queue, char* str);

/*
A nice c "class" that deals with the filesystem. See the bottom of FileSystem.c for example usage.
*/

// These two structures are useful.
typedef struct FileSystem FileSystem;
typedef struct FileSystemIterator FileSystemIterator;

// Ignore these two
typedef struct Folder Folder;
typedef struct File File;

// ******************************************* Filesystem *******************************************

// Creates a new files ystem at the given path
// 	path : (not claimed) The null-terminated path to the directory 
//	ret  : (not claimed) The file system
FileSystem* filesystem_new(char* path);

// Prints the filesystem in a nice way
//	fs : (not claimed) The filesystem to print
void filesystem_print(FileSystem* fs);

// Performs a diff between the provided filesystem and the current state of said filesystem.
//	old        : (not claimed) The previous state of the filesystem.
//	new        : (not claimed) The current state of a filesystem.
//	additions  : (not claimed) After returning, contains a filesystem describing any files that were added.
//	deletions  : (not claimed) After returning, contains a filesystem describing any files that were deleted.
void filesystem_diff(FileSystem* old, FileSystem* new, FileSystem** additions, FileSystem** deletions);

// Subtracts one filesystem1 from filesystem0, filesystem0 -= filesystem1
// filesystem0 : (not claimed) The filesystem that should have things removed from it. THIS IS MODIFIED!!!
// filesystem1 : (not claimed) The other filesystem. Not modified. This argument should come from the
//							   deletions return value of filesystem_diff.
void filesystem_minus_equals(FileSystem* filesystem0, FileSystem* filesystem1);

// Adds one filesystem1 to filesystem0, filesystem0 += filesystem1
// filesystem0 : (not claimed) The filesystem that should have things added to it. THIS IS MODIFIED!!!
// filesystem1 : (not claimed) The other filesystem. Not modified. This argument should come from the
//							   additions return value of filesystem_diff.
void filesystem_plus_equals(FileSystem* filesystem0, FileSystem* filesystem1);

// Performs a deep copy of the given filesystem.
//	filesystem : (not claimed) The filesystem to copy.
//	ret        : (not claimed) The copy of the provided filesystem. 
FileSystem* filesystem_copy(FileSystem* filesystem);

// Gets the root path of the given filesystem.
//	filesystem : (not claimed) The filesystem in question
//	ret        : (claimed) The path to the root directory of the filesystem
char* filesystem_get_root_path(FileSystem* filesystem);

// Sets the root path of the given filesystem.
//	filesystem : (not claimed) The filesystem in question
//	path       : (not claimed) The path to the root directory of the filesystem
void filesystem_set_root_path(FileSystem* filesystem, char* path);

// Serializes the provided filesystem to save it to disk or send over the network.
//	filesystem : (not claimed) The filesystem to serialize
//	data	   : (not claimed) The serialized data
//	length	   : (not claimed) The length of the serialized data
void filesystem_serialize(FileSystem* filesystem, char** data, int* length);

// Takes some data and deserializes it into a filesystem.
//	data      : (not claimed) The data to deserialize
//  bytesRead : (not claimed) The number of bytes read by the deserializer
//	ret	      : (not claimed) The returned filesystem
FileSystem* filesystem_deserialize(char* data, int* bytesRead);

// Destroys a filesystem
//	fs : (claimed) The filesystem to destroy
void filesystem_destroy(FileSystem* fs);

// ******************************************* Iterator *******************************************

// Creates a new iterator for a filesystem. An iterator will sequentially return the paths to
// each of the files and folders contained in it after each call to filesystemiterator_next
//	fs  : (not claimed) The filesystem to be iterated over
//	ret : (not claimed) The iterator
FileSystemIterator* filesystemiterator_new(FileSystem* fs);

// Gets the next path of the filesystem
//	iterator : (not claimed) The iterator being iterated over
//	ret		 : (claimed) The path
char* filesystemiterator_next(FileSystemIterator* iterator, int* length);

// Destroys an iterator
//	iterator : (claimed) The iterator to destroy
void filesystemiterator_destroy(FileSystemIterator* iterator);

#endif

