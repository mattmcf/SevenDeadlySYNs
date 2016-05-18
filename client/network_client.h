/*
 * network_client.h
 *
 * descriptions of functions used by the networking thread on the network_client
 *
 */

 #ifndef _NETWORK_CLIENT_H
 #define _NETWORK_CLIENT_H

/*
 * -- tracker 2 client --
 * tkr_queues_to_client[0] -> transaction update queue (JFS of updated files)
 * tkr_queues_to_client[1] -> file acquisition status queue
 * tkr_queues_to_client[2] -> peer added queue
 * tkr_queues_to_client[3] -> peer deleted queue
 * 
 * -- client 2 client --
 * clt_queues_to_client[0] -> receive request for chunk
 * clt_queues_to_client[1] -> receive chunk from peer (can be error)
 * 
 */ 

typedef struct clt_network_arg {
	AsyncQueue ** tkr_queues_to_client; 	// to logic from tracker
	AsyncQueue ** clt_queues_to_client; 	// to logic from client

	/* 
	 * this queue must do the following things
	 * 	- send a current status update (JFS)
	 * 	- send a request for a file chunk
	 * 	- send a file chunk
	 * 	- send an error (associated with an attempt to get a chunk)
	 * 	- send an update to tracker about acquired chunk
	 * 	- send a "I'm quitting message"
	 */

	AsyncQueue * queue_from_client; 	

	//AsyncQueue ** tkr_queues_from_client; 	// from logic to tracker
	//AsyncQueue ** clt_queues_from_client; 	// from logic to clients
} clt_network_arg;

 #endif _NETWORK_CLIENT_H