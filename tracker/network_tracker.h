/*
 * network_tracker.h
 *
 * descriptions of functions used by the networking thread on the network_tracker
 *
 */

#ifndef _NETWORK_TRACKER_H
#define _NETWORK_TRACKER_H

#include "../utility/FileSystem/FileSystem.h"
#include "../utility/FileTable/FileTable.h"
 
#define SEND_TO_ALL_PEERS -1 	// used for send_peer() call

typedef struct TNT TNT;

TNT * StartTrackerNetwork();

// nicely ends tracker network
void EndTrackerNetwork(TNT * tnt);

/* ###################################
 * 
 * Functions that the tracker logic will call
 *
 * ################################### */

/* ----- receiving ----- */

// Receives a "current state" fs message from client 
//	fs : (not claimed) pointer that will reference deserialized client JFS
// 	clientid : (not claimed) pointer to int that will be filled with client id
// 	ret : (static) 1 is success, -1 is failure (communications broke) 
int receive_client_state(TNT * tnt, FileSystem ** fs, int * clientid);

// sending file acquisition: first 4 bytes (int) is chunk number, rest is filepath
// Receives a "got file chunk" message from client : CLT_2_TKR_CLIENT_GOT
//	path : (not claimed) the filepath of the acquired file
// chunkNum: (not claimed) the chunk number of the file that was acquired
// 	clientid : (not claimed) pointer to int that will be filled with client id if update is present
// 	ret : (static) 1 is success, -1 is failure (communications broke) 
int receive_client_got(TNT * tnt, char * path, int * chunkNum, int * clientid);

// Receive client file system update (modified file) : CLT_2_TKR_FILE_UPDATE
// 	tnt : (not claimed) thread block 
//	clientid : (not claimed) space for client id that will be filled if update is there
// 	ret : (not claimed) total number of bytes deserialized or -1 if no update available
int receive_client_update(TNT * thread, int * clientid, FileSystem ** additions, FileSystem ** deletions);

// client added
// 	tnt : (not claimed) thread block
//	ret : client id (-1 on failure, id on success)
int receive_new_client(TNT * tnt);

// receive notice that client should be removed from network : CLT_2_TKR_REMOVE_CLIENT
// 	tnt : (not claimed) thread block
// 	ret : client id (-1 if no id, id if client should be removed)
int receive_lost_client(TNT * tnt);

// receive client request
// 	tnt : (not claimed) thread block
// 	ret : (static) client id if there's an outstanding request, else -1
int receive_master_request(TNT * tnt);

/* ----- sending ----- */

// Sends a serialized filesystem of diffs to client
// 	fs : (not claimed) pointer to diff FileSystem
// 	clientid : (static) which client to send to
// 	ret : (static) 1 is success, -1 is failure ()
int send_transaction_update(TNT * tnt, FileSystem * additions, FileSystem * deletions, int clientid);

// Send file acquisition update (client got # chunk of @ file)
// 	thread_block : (not claimed) thread_block
//	dest_id : (static) id to send to
//	got_id : (static) client that got 
// 	filename : (not claimed) name of file
//	chunk_num : (static) chunk id that client has acquired
//	ret : 1 on success, -1 on failure
int send_got_chunk_update(TNT * thread_block, int dest_id, int got_id, char * filename, int chunk_num);

// Sends file system update (client updated @ file)
// 	tnt : (not claimed) thread block
// 	destination_client : (static) which client to send to
//	originator_client : (static) which client originated the FS update
// 	additions : (not claimed) additions part of FS to update -- tracker logic must claim when all done
// 	deletions : (not claimed) deletions part of FS to update -- tracker logic must claim when all done
int send_FS_update(TNT * thread_block, int destination_client, int originator_client, FileSystem * additions, FileSystem * deletions);

// send to all peers to notify that a new peer has appeared
int send_peer_added(TNT * tnt, int destination_client_id, int new_client_id);

// send to all peers to notify that peer has disappeared
int send_peer_removed(TNT * tnt, int destination_client_id, int removed_client_id);

// send master JFS to client
// 	tnt : (not claimed) thread block
//	client_id : (not claimed) who to send to
//	fs : (not claimed) FileSystem to send
//	ret : (static) 1 on success, -1 on failure
int send_master(TNT * tnt, int client_id, FileSystem * fs);

// send master FileTable to client
//	tnt : (not claimed) thread block
// 	client_id : (static) client to send to
// 	ft : (not claimed) FileTable to send
// 	ret : (static) 1 on success, -1 on failure
int send_master_filetable(TNT * tnt, int client_id, FileTable * ft);

#endif // _NETWORK_TRACKER_H 
