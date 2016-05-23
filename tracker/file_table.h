/* 
* CS60 Final Project
* DartSync
* FileTable.h
*/

#ifndef _FILE_TABLE_H
#define _FILE_TABLE_H

#include "../utility/ChunkyFile/ChunkyFile.h"

// a table that holds all of the files
typedef struct ChunkyFileTable {
	int numberOfFiles;				// number of files in the table
	ChunkyFileTable_file* head;		// points to the head of the table
	ChunkyFileTable_file* tail;		// points to the tail of the table
} ChunkyFileTable;

// Chunky file table, file information. MAYBE ALSO ADD A TIMESTAMP TO KEEP TRACK OF MOST RECENT
typedef struct ChunkyFileTable_file {
	char* name;						// file name/file path
	int size;						// size of file (this may not matter)
	int numChunks;					// number of chunks in the file
	ChunkyFileTable_chunk* head;	// head of singly linked list of chunks
	ChunkyFileTable_chunk* tail;	// head of singly linked list of chunks
	ChunkyFileTable_file* next;		// next file in singly linked list of files
} ChunkyFileTable_file;

// file chunk
typedef struct ChunkyFileTable_chunk {
	int chunkNum;					// the chunk number of this chunk
	int numberOfPeers;				// number of peers that have the file
	int* peerID;					// an array of the peers that have this chunk
	ChunkyFileTable_chunk* next;	// singly linked list, point to next chunk
} ChunkyFileTable_chunk;

// create a new file table
// ret: (not claimed) newly allocated table
ChunkyFileTable* filetable_createTable();

// create a new file 
// ret: (not claimed) newly allocated file
ChunkyFileTable_file* filetable_createFile(char* path, int size, int numChunks, int peerID);

// create a new file table
// file: (claimed) file to destroy
// ret: (static) 1 if success, -1 if failure
int filetable_destroyFile(ChunkyFileTable_file* file);

// create a new file table
// table: (claimed) table to destroy
// ret: (static) 1 if success, -1 if failure
int filetable_destroyTable(ChunkyFileTable* table);

// saves a new file that has been created to the table
//	path: (not claimed) a file path to use as a descriptor/identifier for the new file
//	size: (not claimed) the size of the file
//	numChunks: (not claimed) the number of chunks needed for the file (THIS MAY NOT BE NECESSARY, can calculate in add)
//	peerID: (not claimed) the peerID that made the file. should be added as owning all chunks
// 	ret: (static) 1 on success, -1 on failure
int filetable_addFile(char* path, int size, int numChunks, int peerID, ChunkyFileTable* table);

// removes a file from the file table 
// path: (not claimed) a file path to use as a descriptor/identifier for the file
// 	ret: (static) 1 on success, -1 on failure
int filetable_removeFile(char* path);

// a client updated a file, clear old file instance, make new one
//	path: (not claimed) a file path to use as a descriptor/identifier for the new file
//	size: (not claimed) the size of the file
//	numChunks: (not claimed) the number of chunks needed for the file (THIS MAY NOT BE NECESSARY, can calculate in add)
//	peerID: (not claimed) the peerID that made the file. should be added as owning all chunks
// 	ret: (static) 1 on success, -1 on failure
int filetable_fileUpdated(char* path, int size, int numChunks, int peerID);

// a peer successfully downloaded a chunk of a file
//	path: (not claimed) a file path to use as a descriptor/identifier for the new file
// chunkNum: (not claimed) the chunk number that was downloaded (ie for chunk 2 of 4, input 2)
//	peerID: (not claimed) the peerID that made the file. should be added as owning all chunks
// 	ret: (static) 1 on success, -1 on failure
int filetable_peerDownloadSuccess(char* path, int chunkNum, int peerID);

#endif // _FILE_TABLE_H 

