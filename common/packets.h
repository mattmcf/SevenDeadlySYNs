/*
 * packets.h
 *
 * defines the communication packets that tracker and clients will use to communicate
 *
 */

#ifndef _PACKETS_H
#define _PACKETS_H

#include "peer_table.h"

/*
 * How to send a packet with an arbitrary amount of data attached:
 * 	1) send type + data len
 * 	2) send data buffer 
 *
 * Splitting up packet is necessary because we can't allocate a struct that will
 * be guaranteed to hold all of the data. 
 *
 * On the read side:
 *	1) recv() header data into a type field and a size field
 * 	2) recv() data into an allocated buffer of data_len size
 */

typedef enum {
	CLIENT_STATE,
	CLIENT_UPDATE,
} client_to_tracker_t;

typedef enum {
	TRANSACTION_UPDATE,
	FILE_ACQ_UPDATE,
	PEER_ADDED,
	PEER_DELETED,
} tracker_to_client_t;

/*
 * data field will be structured as follows:
 * 	TRANSACTION_UPDATE -> a serialized FileSystem
 * 	FILE_ACQ_UPDATE -> ???
 * 	PEER_ADDED -> N entries of peer_t where N = len / sizeof(peer_t)
 * 	PEER_DELETED -> N ints (ids) to delete where N = len / sizeof(int)
 */

typedef struct tracker_pkt {
	tracker_to_client_t type;
	int data_len;
	char * data;
} tracker_pkt_t;


#endif // _PACKETS_H