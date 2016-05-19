/*
 * network_tracker.h
 *
 * descriptions of functions used by the networking thread on the network_tracker
 *
 */

#ifndef _NETWORK_TRACKER_H
#define _NETWORK_TRACKER_H

#include "../utility/FileSystem/FileSystem.h"

typedef struct TNT TNT;

TNT * StartTrackerNetwork();

// nicely ends tracker network
void EndNetwork();

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

// client deleted

/* ----- sending ----- */

// Sends a serialized filesystem of diffs to client
// 	fs : (not claimed) pointer to diff FileSystem
// 	clientid : (static) which client to send to
// 	ret : (static) 1 is success, -1 is failure ()
int send_transaction_update(TNT * tnt, FileSystem * fs, int clientid);

// Sends file system update
int send_FS_update();

// send to all peers to notify that a new peer has appeared
int send_peer_added();

// send to all peers to notify that peer has disappeared
int send_peer_removed();


#endif // _NETWORK_TRACKER_H 
