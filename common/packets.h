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
 * 	2) send data  
 *
 * Splitting up packet is necessary because we can't allocate a struct that will
 * be guaranteed to hold all of the data. 
 *
 * On the read side:
 *	1) recv() header data into a type field and a size field
 * 	2) recv() data into an allocated buffer of data_len size
 */

/*
 * data field will be structured as follows:
 * 	TRANSACTION_UPDATE -> a serialized FileSystem
 * 	FILE_ACQ_UPDATE -> ???
 * 	PEER_ADDED -> N entries of peer_t where N = len / sizeof(peer_t)
 * 	PEER_DELETED -> N ints (ids) to delete where N = len / sizeof(int)
 */

/* ----- TRACKER TO CLIENT ----- */

typedef enum {
	TRANSACTION_UPDATE,
	FILE_ACQ_UPDATE,
	FS_UPDATE,
	MASTER_STATUS,
	PEER_ADDED,
	PEER_DELETED,
	PEER_TABLE,
} tracker_to_client_t;

typedef struct tracker_pkt {
	tracker_to_client_t type;
	int data_len;
	//char * data;
} tracker_pkt_t;

// use this struct to put data onto the queue from logic
typedef struct tracker_data {
	int client_id;
	int data_len;
	void * data;
} tracker_data_t;

/* ----- CLIENT TO TRACKER ----- */

typedef enum {
	HEARTBEAT,	
	CLIENT_STATE,
	CLIENT_UPDATE,
	REQUEST_MASTER,
} client_to_tracker_t;

typedef struct client_pkt {
	client_to_tracker_t type;
	int data_len;
	//char * data;
} client_pkt_t;

// Use this struct to put data onto the queue to logic
typedef struct client_data {
	int client_id;
	int data_len;
	void * data;
} client_data_t;

/* ----- CLIENT TO CLIENT ----- */

// client to client queues
typedef struct chunk {
	int client_id;
	int chunk_num;
	int file_str_len;
	char * file_name;
	int data_len;
	char * data;
} chunk_data_t;

// client to client packets
typedef enum {
	REQUEST_CHUNK,
	CHUNK,
	ERROR,
} client_to_client_t;

typedef struct c2c_pkt {
	client_to_client_t type;
	int src_client;
	int chunk_num;
	int file_str_len;
	int data_len;
	//char * file_str -> will send next
	//char * data; 	-> will send next
} c2c_pkt_t;

#endif // _PACKETS_H
