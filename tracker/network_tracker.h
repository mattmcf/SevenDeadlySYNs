/*
 * network_tracker.h
 *
 * descriptions of functions used by the networking thread on the network_tracker
 *
 */

#ifndef _NETWORK_TRACKER_H
#define _NETWORK_TRACKER_H

#include "../utility/AsyncQueue.h"
#include "../utility/FileSystem.h"

typedef struct TrackerNetworkThread TrackerNetworkThread;

TrackerNetworkThread * StartTrackerNetwork();

/* ###################################
 * 
 * Functions that the client logic will call
 *
 * ################################### */

/* ----- receiving ----- */

// 

/*
 * queues_to_tracker[0] -> a new client is trying to join, contains current JFS state (to logic)
 * queues_to_tracker[1] -> client has just updated a file (to logic)
 * queues_to_tracker[2] -> client disconnected queue (to logic)
 * queues_to_tracker[3] -> client successful get queue (to logic)
 * queues_to_tracker[4] -> client failed to get queue (to logic)
 *
 * queues_from_tracker[0] -> transaction update queue
 * queues_from_tracker[1] -> file acquisition status update queue
 * queues_from_tracker[2] -> peer added messages to distribute
 * queues_from_tracker[3] -> peer removed messages to distribute
 */

typedef struct tkr_network_arg {
	AsyncQueue ** queues_to_tracker; 		// to logic
	AsyncQueue ** queues_from_tracker;  	// from logic
}


// starts network thread which polls queues from tracker logic as well as incoming connections from peers
void * trkr_network_start(void * arg);

/* ###################### *
 * 
 * Functions callable by the tracker's main thread
 *
 * ###################### */

// Receives a "current state" fs message from client 
//	fs : (not claimed) pointer that will reference deserialized client JFS
// 	clientid : (not claimed) pointer to int that will be filled with client id
// 	ret : (static) 1 is success, -1 is failure (communications broke) 
int receive_client_state(FileSystem ** fs, int * clientid);

// Sends a serialized filesystem of diffs to client
// 	fs : (not claimed) pointer to diff FileSystem
// 	clientid : (static) which client to send to
// 	ret : (static) 1 is success, -1 is failure ()
int send_transaction_update(FileSystem * fs, int clientid);

//
int send_FS_update();

// send to all peers to notify that a new peer has appeared
send_peer_added();

// send to all peers to notify that peer has disappeared
send_peer_removed();


#endif // _NETWORK_TRACKER_H 