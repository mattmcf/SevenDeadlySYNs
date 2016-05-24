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

// receive request for chunk 

// receive chunk from peer 


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

// send request for chunk to peer

// send chunk to client

// send chunk request error response


 #endif // _NETWORK_CLIENT_H
