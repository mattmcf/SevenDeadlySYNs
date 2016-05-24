/*
* Dartmouth College CS60 Final Project
* DartSync
*
*/

#ifndef _TRACKER_H
#define _TRACKER_H


#include "network_tracker.h"
#include "../utility/FileSystem/FileSystem.h"
#include "../utility/ChunkyFile/ChunkyFile.h"

// Peer table structure
typedef struct peer_table {
  int numberOfEntries;		        //number of peers in network
  int *peerIDs;						//peerID number
} peer_table;

// // Chunky file table, file information
// typedef struct ChunkyFileTable_file {
// 	char* name;						// file name/file path
// 	int size;						// size of file (this may not matter)
// 	int numChunks;					// number of chunks in the file
// 	ChunkyFileTable_chunk* head;	// head of singly linked list of chunks
// } ChunkyFileTable_file;

// // file chunk
// typedef struct ChunkyFileTable_chunk {
// 	int chunkNum;					// the chunk number of this chunk
// 	int* peerID;					// an array of the peers that have this chunk
// 	ChunkyFileTable_chunk* next;	// singly linked list, point to next chunk
// } ChunkyFileTable_chunk;


void intHandler(int dummy);

// ceates a new peer table. allocates the memory
peer_table* createPeerTable();

// destroys peer table and frees memory
int destroyPeerTable(peer_table* deleteTable);

// Takes a new peer and adds them to a table based on unique ID
int addPeerToTable(int peerID);

// Removes peer after they leave the network
int removePeerFromTable(int peerID);

// print the peer table
int printPeerTable();

//close everything, free memory
int closeTracker();

// peer successfully retrieved master. Update file table
// int clientFileRetrieveSuccess(int peerID, int fileID);
/*
update file system and peer table to reflect that the peer now has new file
*/


// identifies files that the peer needs and sends table of files and clients
int sendUpdates(int peerID);
/*
compare peer file system to saved system
find difference
send difference
*/
// broadcast to all peers that there is a new peer
int newPeerBroadcast(int newPeerID, TNT *network);



#endif // _TRACKER_H 
