/*
 * network_client.h
 *
 * descriptions of functions used by the networking thread on the network_client
 *
 */

 #ifndef _NETWORK_CLIENT_H
 #define _NETWORK_CLIENT_H

#define IP_MAX_LEN 16



typedef struct ClientNetworkThread ClientNetworkThread;

ClientNetworkThread * StartClientNetwork(char * ip_addr, int ip_len);

/* ###################################
 * 
 * Functions that the client logic will call
 *
 * ################################### */

/* ----- receiving ----- */

// receive transaction update

// receive acq status update

// get peer added message

// get peer deleted message

// receive request for chunk

// receive chunk from peer

/* ----- sending ----- */

/* 
 * this queue must do the following things
 * 	- send a current status update (JFS)
 * 	- send a request for a file chunk
 * 	- send a file chunk
 * 	- send an error (associated with an attempt to get a chunk)
 * 	- send an update to tracker about acquired chunk
 * 	- send a "I'm quitting message"
 */

 #endif _NETWORK_CLIENT_H