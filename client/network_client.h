/*
 * network_client.h
 *
 * descriptions of functions used by the networking thread on the network_client
 *
 */

#ifndef _NETWORK_CLIENT_H
#define _NETWORK_CLIENT_H

#include "../utility/FileSystem/FileSystem.h"

#define IP_MAX_LEN 16

typedef struct CNT CNT;

// starts client network
CNT * StartClientNetwork(char * ip_addr, int ip_len);

// nicely ends network
void EndClientNetwork(CNT * thread_block);

/* ###################################
 * 
 * Functions that the client logic will call
 *
 * ################################### */

/* ----- receiving ----- */

// receive transaction update
// 	cnt : (not claimed) thread block
// 	additions : (not claimed) JFS of additions
// 	deletions : (not claimed) JFS of deletions
//	ret : (static) 1 on success, -1 on failure 
int recv_diff(CNT * cnt, FileSystem ** additions, FileSystem ** deletions);

// receive acq status update

// receive acq status update 

// receive peer added message
//	thread_block : (not claimed) thread block
// 	ret : (static) id of added client (always > 1) or -1 if no new clients
int recv_peer_added(CNT * thread_block);

// receive peer deleted message
// 	thread_block : (not claimed) thread block
// 	ret : (static) if of a deleted client (> 1) or -1 if no new clients
int recv_peer_deleted(CNT * thread_block);

// receive master JFS from tracker 
// 	CNT : (not claimed) thread block
// 	received_length : (not claimed) will be filled in with deserialized bytes if update was present
//	ret : (not claimed) master file system
//				null if no update
FileSystem * recv_master(CNT * thread, int * received_length);

/* poll the network to see if we have received a chunk request
 * from a peer client 
 * 	cnt - the current state of the network
 * 	filepath - an unallocated pointer to a filepath for the chunky file
 *	peer_id - the peer that made the request 
 * 	buf - the chunk that was requested
 * 		TODO - figure out how to combine this with chunky file and
 *		how to expand on it to allow for transerring an entire file
 	len - the expected length (DO WE NEED THIS)
 */
int receive_chunk_request(CNT *cnt, char **filepath, int *peer_id, int *chunk_id, int *len);

/* client calls this to receive a chunk update from the network 
 * 	cnt - the current state of the network
 * 	(claimed) ret - the chunk that we received
 */
char* receive_chunk(CNT *cnt);

/* ----- sending ----- */

// send current status
int send_status(CNT * thread, FileSystem * fs); 

// send file acquistion update

// send quit message to tracker 
// 	thread : (not claimed) thread block
//	additions : (not claimed) addition FS - claim after call
// 	deletions : (not claimed) deletions FS - claim after call
//	ret : 1 on success, -1 on failure
int send_updated_files(CNT * thread, FileSystem * additions, FileSystem * deletions);

// send request for master JFS to tracker
// returns 1 on success, -1 on failure
int send_request_for_master(CNT * cnt);

/* send a chunk request from the client app to a peer client 
 * 	cnt - the current state of the network
 * 	filepath - a pointer to a filepath for the chunky file
 *	peer_id - the peer to request the chunk from 
 * 	buf - the chunk that we are requesting
 * 		TODO - figure out how to combine this with chunky file and
 * 		how to expand on it to allow for transerring an entire file
 	len - the expected length (DO WE NEED THIS)
 */
int send_chunk_request(CNT *cnt, char *filepath, int peer_id, int chunk_id, int len);

/* client calls this to send a chunk update to another client 
 * 	cnt - current state of the network 
 * 	peer_id - the peer that we are sending it to
 *	(claimed) buf	- the chunk that we are sending
 * 	len - the length of the chunk that we are sending 
 */
int send_chunk(CNT *cnt, int peer_id, char *buf, int len);

// send chunk request error response
int send_chunk_rejection(CNT * cnt, char *filepath, int peer_id, int chunk_id);
	return -1;
}

 #endif // _NETWORK_CLIENT_H
