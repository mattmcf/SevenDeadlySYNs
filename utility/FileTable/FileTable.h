#ifndef FILETABLE_H
#define FILETABLE_H

#include "../HashTable/HashTable.h"
#include "../Queue/Queue.h"
#include "../FileSystem/FileSystem.h"
#include "../ChunkyFile/ChunkyFile.h"

typedef struct FileTable FileTable;

// Creates a new file table
//	ret	: (not claimed) the new file table
FileTable* filetable_new();

// Destroys a file table
//	filetable : (claimed) the table to destroy
void filetable_destroy(FileTable* filetable);

// Serializes the provided filesystem to save it to disk or send over the network.
//	filetable  : (not claimed) The filetable to serialize
//	data	   : (not claimed) The serialized data
//	length	   : (not claimed) The length of the serialized data
void filetable_serialize(FileTable* filetable, char** data, int* length);

// Takes some data and deserializes it into a filetable.
//	data      : (not claimed) The data to deserialize
//  bytesRead : (not claimed) The number of bytes read by the deserializer
//	ret	      : (not claimed) The returned filetable
FileTable* filetable_deserialize(char* data, int* bytesRead);

// Adds the files from the given file system to the file table and sets all of them to have the given peer. (overwrites previous entries)
//	filetable : (not claimed) The file table to add to
//	filesystem : (not claimed) The filesystem to add
//	peer : (static) The peer to initialize all the added files to
void filetable_add_filesystem(FileTable* filetable, FileSystem* filesystem, int peer);

// Removes all the files from the provided filesystem from the file table
//	filetable : (not claimed) The file table to remove from
//	filesystem : (not claimed) The files to remove
void filetable_remove_filesystem(FileTable* filetable, FileSystem* filesystem);

// Get a queue of the peers who have the given chunk of the given file
//	filetable : (not claimed) The file table to get the queue from
//	path : (not claimed) The file
//	chunk : (static) The chunk
//	ret : (claimed) The queue of peers that have the given chunk (or NULL if problem)
Queue* filetable_get_peers_who_have_file_chunk(FileTable* filetable, char* path, int chunk);

// Set that a specific chunk of a specific file has been acquired by a peer.
//	filetable	: (not claimed) The file table
//	path		: (not claimed) The path to the file
//	peer		: (not claimed) The peer to add
//	chunk		: (not claimed) The chunk of the file
void   filetable_set_that_peer_has_file_chunk(FileTable* filetable, char* path, int peer, int chunk);

// Wipe a specific peer from the file table (when the peer disconnects)
//	filetable	: (not claimed) The file table
//	id			: (static) The peer to remove
void   filetable_remove_peer(FileTable* filetable, int id);

// Prints the file table. 
void filetable_print(FileTable* filetable);

ChunkyFile* filetable_get_chunkyfile(FileTable* filetable, char* path);
void filetable_set_chunkyfile(FileTable* filetable, char* path, ChunkyFile* file);

/* ------------------ ITERATOR ------------------ */

typedef struct FileTableIterator FileTableIterator;

FileTableIterator* filetableiterator_new(FileTable* filetable);
char * filetableiterator_path_next(FileTableIterator* iterator);
void filetableiterator_destroy(FileTableIterator* iterator);

#endif
