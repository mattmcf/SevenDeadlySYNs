/*
 * network_tracker.h
 *
 * descriptions of functions used by the networking thread on the network_tracker
 *
 */

#ifndef _NETWORK_TRACKER_H
#define _NETWORK_TRACKER_H

#include "../utility/FileSystem/FileSystem.h"

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

// client file system update (got and failed to get)
// 	tnt : (not claimed) thread block 
//	clientid : (not claimed) space for client id that will be filled if update is there
// 	ret : (not claimed) client's update JFS (new minus original)
FileSystem * receive_client_update(TNT * tnt, int * clientid);

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

// Sends file system update
int send_FS_update(TNT * tnt);

// send to all peers to notify that a new peer has appeared
int send_peer_added(TNT * tnt);

// send to all peers to notify that peer has disappeared
int send_peer_removed(TNT * tnt);

// send master JFS to client
int send_master(TNT * tnt, int client_id, FileSystem * fs);

#endif // _NETWORK_TRACKER_H 
