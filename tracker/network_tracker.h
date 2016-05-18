/*
 * network_tracker.h
 *
 * descriptions of functions used by the networking thread on the network_tracker
 *
 */

#ifndef _NETWORK_TRACKER_H
#define _NETWORK_TRACKER_H

#include "../utility/AsyncQueue.h"

typedef struct tkr_network_arg {
	AsyncQueue ** queues;
	
}

// starts network thread which polls queues from tracker logic as well as incoming connections from peers
void * tkr_network_start(void * arg);

/* ###################### *
 * 
 * Functions callable by the tracker's main thread
 *
 * ###################### */

send_transaction_update();

send_FS_update();

send_peer_added();

send_peer_removed();


#endif // _NETWORK_TRACKER_H 