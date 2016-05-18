/*
 * network_tracker.c
 *
 * contains functionality for the tracker's network thread
 */



/* ###################### *
 * 
 * Working loop...
 *
 * ###################### */

void * tkr_network_start(void * arg) {

	// open connection on listening port...

		// wait for incoming peer connections 
		// handle inquisitive peers when they connect 
		// -> put message on queue to app logic that client needs to initiate handshake

	// continue to poll open thread connections
		// "heartbeat messages"
		// receive transaction update messages from peers
}