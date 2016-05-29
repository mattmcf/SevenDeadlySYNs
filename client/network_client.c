/*
 * network_client.c
 *
 * contains functionality for client-side network thread
 */

#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <strings.h>
#include <arpa/inet.h>
#include <signal.h>
#include <netdb.h>
#include <assert.h>
#include <sys/utsname.h>
#include <pthread.h>
#include <unistd.h>
#include <netdb.h>

#include "network_client.h"
#include "../common/constant.h"
#include "../common/peer_table.h"
#include "../common/packets.h"
#include "../utility/AsyncQueue/AsyncQueue.h" 
#include "../utility/FileSystem/FileSystem.h" 
#include "../utility/HashTable/HashTable.h"
#include "../utility/FileTable/FileTable.h"
#include "../utility/ColoredPrint/ColoredPrint.h"

#define INIT_PEER_SIZE 10

/* --- 	queue definitions --- */
//#define TKR_2_ME_TRANSACTION_UPDATE 0
#define TKR_2_ME_FS_UPDATE 0 
#define TKR_2_ME_FILE_ACQ 1
#define TKR_2_ME_PEER_ADDED 2
#define TKR_2_ME_PEER_DELETED 3
#define TKR_2_ME_RECEIVE_MASTER_JFS 4
#define TKR_2_ME_RECEIVE_MASTER_FT 5

#define CLT_2_ME_REQUEST_CHUNK 0
#define CLT_2_ME_RECEIVE_CHUNK 1

#define ME_2_TKR_CUR_STATUS 0
#define ME_2_TKR_ACQ_UPDATE 1
#define ME_2_TKR_UPDATED_FILE_DIFF 2
#define ME_2_TKR_GET_MASTER 3

#define ME_2_CLT_REQ_CHUNK 0
#define ME_2_CLT_SEND_CHUNK 1
#define ME_2_CLT_SEND_ERROR 2

int err_format = -1;
int network_format = -1;

typedef struct _CNT {

	/*
	 * *** INCOMING QUEUES ***
	 * 
	 * -- tracker 2 client --
	 * tkr_queues_to_client[0] -> FileSystem update queue
	 * tkr_queues_to_client[1] -> file acquisition status queue
	 * tkr_queues_to_client[2] -> peer added queue
	 * tkr_queues_to_client[3] -> peer deleted queue
	 * tkr_queues_to_client[4] -> master JFS
	 * tkr_queues_to_client[5] -> master file table
	 * 
	 * -- client 2 client --
	 * clt_queues_to_client[0] -> receive request for chunk
	 * clt_queues_to_client[1] -> receive chunk from peer (can be error)
	 * 
	 */
	AsyncQueue ** tkr_queues_to_client; 	// to logic from tracker
	AsyncQueue ** clt_queues_to_client; 	// to logic from client

	/*
	 * *** OUTGOING QUEUES ***
	 * 
	 * -- client to tracker --
	 * tkr_queues_from_client[0] -> send current status / (join network)
	 * tkr_queues_from_client[1] -> send acquisition update to tracker (successful & unsucessful)
	 * tkr_queues_from_client[2] -> send updated file diff
	 * tkr_queues_from_client[3] -> send request for master
	 *
	 * -- client 2 client --
	 * clt_queues_from_client[0] -> send request for chunk
	 * clt_queues_from_client[1] -> send chunk
	 * clt_queues_from_client[2] -> send an error to client
	 *
	 */
	AsyncQueue ** tkr_queues_from_client; 	// from logic to tracker
	AsyncQueue ** clt_queues_from_client; 	// from logic to client

	/*
	 * network monitoring information
	 */
	pthread_t thread_id;
	char * ip_addr;
	int ip_len;
	int tracker_fd;
	int peer_listening_fd;

	peer_table_t * peer_table;

	HashTable * request_table;

} _CNT_t;

// network thread start point
void * clt_network_start(void * arg);

// structures for connection record table
typedef struct connection_record {
	int client_id;
	int outstanding_requests;
} conn_rec_t;

int ConnRecHash(void * target) {
	conn_rec_t * entry = (conn_rec_t *)target;
	return entry->client_id;
}

int ConnEqualsFunction(void * e0, void * e1) {
	conn_rec_t * entry0 = (conn_rec_t *)e0;
	conn_rec_t * entry1 = (conn_rec_t *)e1;
	return (entry0->client_id == entry1->client_id);
}

/* ################################################## *
 * 
 * Starting and ending network thread functions
 *
 * ################################################## */

CNT * StartClientNetwork(char * ip_addr, int ip_len) {

	if (!ip_addr) {
		fprintf(stderr,"StartClientNetwork: bad ip address\n");
		return NULL;
	}

	_CNT_t * client_thread = (_CNT_t *)calloc(1,sizeof(_CNT_t));

	/* -- incoming client to tracker -- */
	client_thread->tkr_queues_to_client = (AsyncQueue **)calloc(6, sizeof(AsyncQueue *));
	client_thread->tkr_queues_to_client[TKR_2_ME_FS_UPDATE] = asyncqueue_new();
	client_thread->tkr_queues_to_client[TKR_2_ME_FILE_ACQ] = asyncqueue_new();
	client_thread->tkr_queues_to_client[TKR_2_ME_PEER_ADDED] = asyncqueue_new();
	client_thread->tkr_queues_to_client[TKR_2_ME_PEER_DELETED] = asyncqueue_new();
	client_thread->tkr_queues_to_client[TKR_2_ME_RECEIVE_MASTER_JFS] = asyncqueue_new();
	client_thread->tkr_queues_to_client[TKR_2_ME_RECEIVE_MASTER_FT] = asyncqueue_new();

	/* -- incoming client to client -- */
	client_thread->clt_queues_to_client = (AsyncQueue **)calloc(2,sizeof(AsyncQueue *));
	client_thread->clt_queues_to_client[CLT_2_ME_REQUEST_CHUNK] = asyncqueue_new();
	client_thread->clt_queues_to_client[CLT_2_ME_RECEIVE_CHUNK] = asyncqueue_new();

	/* -- outgoing client to tracker -- */
	client_thread->tkr_queues_from_client = (AsyncQueue **)calloc(4,sizeof(AsyncQueue *));
	client_thread->tkr_queues_from_client[ME_2_TKR_CUR_STATUS] = asyncqueue_new();
	client_thread->tkr_queues_from_client[ME_2_TKR_ACQ_UPDATE] = asyncqueue_new();
	client_thread->tkr_queues_from_client[ME_2_TKR_UPDATED_FILE_DIFF] = asyncqueue_new();
	client_thread->tkr_queues_from_client[ME_2_TKR_GET_MASTER] = asyncqueue_new();

	/* -- outgoing client to client -- */
	client_thread->clt_queues_from_client = (AsyncQueue **)calloc(3,sizeof(AsyncQueue *));
	client_thread->clt_queues_from_client[ME_2_CLT_REQ_CHUNK] = asyncqueue_new();
	client_thread->clt_queues_from_client[ME_2_CLT_SEND_CHUNK] = asyncqueue_new();
	client_thread->clt_queues_from_client[ME_2_CLT_SEND_ERROR] = asyncqueue_new();

	/* -- save tracker server arguments -- */
	client_thread->ip_addr = malloc(ip_len);
	memcpy(client_thread->ip_addr, ip_addr, ip_len);
	client_thread->ip_len = ip_len;

	client_thread->peer_table = init_peer_table(INIT_PEER_SIZE);
	if (!client_thread->peer_table) {
		fprintf(stderr,"client network thread failed to create peer table\n");
		return NULL;
	}

	client_thread->request_table = hashtable_new(&ConnRecHash, &ConnEqualsFunction);

	if (err_format < 0)
	{
		FORMAT_ARG err_arr[] = {COLOR_L_RED, COLOR_BOLD, 0};
		err_format = register_format(err_arr);

		FORMAT_ARG network_arr[] = {COLOR_D_CYAN, 0};
		network_format = register_format(network_arr);
	}

	/* -- spin off network thread -- */
	pthread_create(&client_thread->thread_id, NULL, clt_network_start, client_thread);

	return (CNT *)client_thread;
}

// nicely ends network
void EndClientNetwork(CNT * thread_block) {
	format_printf(network_format, "NETWORK -- cleaning up\n");

	_CNT_t * cnt = (_CNT_t *)thread_block;
	if (!cnt)
		return;

	asyncqueue_destroy(cnt->tkr_queues_to_client[TKR_2_ME_FS_UPDATE]);
	asyncqueue_destroy(cnt->tkr_queues_to_client[TKR_2_ME_FILE_ACQ]);
	asyncqueue_destroy(cnt->tkr_queues_to_client[TKR_2_ME_PEER_ADDED]);
	asyncqueue_destroy(cnt->tkr_queues_to_client[TKR_2_ME_PEER_DELETED]);
	asyncqueue_destroy(cnt->tkr_queues_to_client[TKR_2_ME_RECEIVE_MASTER_JFS]);
	asyncqueue_destroy(cnt->tkr_queues_to_client[TKR_2_ME_RECEIVE_MASTER_FT]);
	free(cnt->tkr_queues_to_client);

	asyncqueue_destroy(cnt->clt_queues_to_client[CLT_2_ME_REQUEST_CHUNK]);
	asyncqueue_destroy(cnt->clt_queues_to_client[CLT_2_ME_RECEIVE_CHUNK]);
	free(cnt->clt_queues_to_client);

	asyncqueue_destroy(cnt->tkr_queues_from_client[ME_2_TKR_CUR_STATUS]);
	asyncqueue_destroy(cnt->tkr_queues_from_client[ME_2_TKR_ACQ_UPDATE]);
	asyncqueue_destroy(cnt->tkr_queues_from_client[ME_2_TKR_UPDATED_FILE_DIFF]);
	asyncqueue_destroy(cnt->tkr_queues_from_client[ME_2_TKR_GET_MASTER]);
	free(cnt->tkr_queues_from_client);

	asyncqueue_destroy(cnt->clt_queues_from_client[ME_2_CLT_REQ_CHUNK]);
	asyncqueue_destroy(cnt->clt_queues_from_client[ME_2_CLT_SEND_CHUNK]);
	asyncqueue_destroy(cnt->clt_queues_from_client[ME_2_CLT_SEND_ERROR]);
	free(cnt->clt_queues_from_client);

	free(cnt->ip_addr);

	// close all peer connections
	for (int i = 0; i < cnt->peer_table->size; i++) {
		if (cnt->peer_table->peer_list[i] && cnt->peer_table->peer_list[i]->socketfd > 0) 
			close(cnt->peer_table->peer_list[i]->socketfd);
	}

	destroy_table(cnt->peer_table);

	hashtable_apply(cnt->request_table, &free);
	hashtable_destroy(cnt->request_table);

	close(cnt->tracker_fd);
	close(cnt->peer_listening_fd);

	free(cnt);

	format_printf(network_format,"NETWORK -- ending client network thread after cleaning up\n");
	
	return;
}

/* ###################### *
 * 
 * Things that the client should do to interact with network thread
 *
 * ###################### */

/* ----- receiving ----- */

// receive file system update: TKR_2_ME_FS_UPDATE
// 	thread : (not claimed) thread_block
// 	additions : (not claimed) will fill in with addition part of FS
// 	additions : (not claimed) will will in with deletion part of FS
//	author_id : (not claimed) will fill with diff's author id
// 	ret : (static) 1 if diff was present, -1 if diff wasn't present
int recv_diff(CNT * thread, FileSystem ** additions, FileSystem ** deletions, int * author_id) {
	if (!thread || !additions || !deletions || !author_id) {
		return -1;
	}

	_CNT_t * cnt = (_CNT_t *) thread;
	int len1 = 0, len2 = 0;

	client_data_t * queue_item = asyncqueue_pop(cnt->tkr_queues_to_client[TKR_2_ME_FS_UPDATE]);
	if (queue_item != NULL) {

		*additions = filesystem_deserialize((char *)queue_item->data, &len1);
		*deletions = filesystem_deserialize(((char *)(long)queue_item->data + (long)len1), &len2);
		*author_id = *(int *)((long)queue_item->data + (long)len1 + (long)len2); 

		free(queue_item->data);
		free(queue_item);
	}

	return (len1 > 0) ? 1 : -1;
}

// receive acq status update : TKR_2_ME_FILE_ACQ
// 	thread_block : (not cliamed) thread_block
// 	client_id : (not claimed) filled in with client id
// 	filename : (not claimed) filled in with pointer to file string
// 	chunk_num : (not claimed) filled in with chunk id
// 	ret : (static) 1 on chunk received notication, -1 in no update
int receive_chunk_got(CNT * thread_block, int * client_id, char ** filename, int * chunk_num) {
	if (!thread_block || !client_id || !filename || !chunk_num) {
		format_printf(err_format, "receive_chunk_got error: null arguments\n");
		return -1;
	}

	_CNT_t * cnt = (_CNT_t *)thread_block;
	client_data_t * queue_item = (client_data_t *)asyncqueue_pop(cnt->tkr_queues_to_client[TKR_2_ME_FILE_ACQ]);
	if (queue_item != NULL) {

		*client_id = queue_item->client_id;
		memcpy(chunk_num, queue_item->data, sizeof(int));
		*filename = strdup((char *)((long)queue_item->data + (long)sizeof(int)));

		free(queue_item->data);
		free(queue_item);
	}

	return (queue_item != NULL) ? 1 : -1;
}

// receive peer added message : TKR_2_ME_PEER_ADDED
//	thread_block : (not claimed) thread block
// 	ret : (static) id of added client (always > 1) or -1 if no new clients
int recv_peer_added(CNT * thread_block) {
	_CNT_t * cnt = (_CNT_t *)thread_block;
	int new_id = (int)(long)asyncqueue_pop(cnt->tkr_queues_to_client[TKR_2_ME_PEER_ADDED]);
	return (new_id > 0) ? new_id : -1;
}

// receive peer deleted message : TKR_2_ME_PEER_DELETED
// 	thread_block : (not claimed) thread block
// 	ret : (static) if of a deleted client (> 1) or -1 if no new clients
int recv_peer_deleted(CNT * thread_block) {
	_CNT_t * cnt = (_CNT_t *)thread_block;
	int deleted_id = (int)(long)asyncqueue_pop(cnt->tkr_queues_to_client[TKR_2_ME_PEER_DELETED]);
	return (deleted_id > 0) ? deleted_id : -1;
}

// receive master JFS from tracker : TKR_2_ME_RECEIVE_MASTER_JFS
// 	CNT : (not claimed) thread block
// 	length_deserialized : (not claimed) will be filled in with length deserialized if there was an update
//	ret : (not claimed) master file system -- needs to be claimed 
//				null if no update
FileSystem * recv_master(CNT * thread, int * length_deserialized) {
	_CNT_t * cnt = (_CNT_t *)thread;
	client_data_t * queue_item = asyncqueue_pop(cnt->tkr_queues_to_client[TKR_2_ME_RECEIVE_MASTER_JFS]);

	FileSystem * fs = NULL;
	if (queue_item) {
		printf("LOGIC -- Receiving master off of queue\n");

		// deserialized in handle_client_message() side
		// fs = filesystem_deserialize(queue_item->data, length_deserialized);
		fs = queue_item->data;
		filesystem_print(fs);

		//free(queue_item->data); // freed in handle_client_message() side
		free(queue_item);
	}

 	return fs;
}

// receives a master file table from the tracker : TKR_2_ME_RECEIVE_MASTER_FT
// 	thread_block : (not claimed) thread block
//	length_deserialized : (not claimed) will be filled in with # of bytes deserialized
// 	ret : (not claimed) master file table if one is present, NULL if not available
FileTable * recv_master_ft(CNT * thread_block, int * length_deserialized) {
	if (!thread_block || !length_deserialized) {
		format_printf(err_format, "recv_master_ft error: null arguments\n");
		return NULL;
	}

	_CNT_t * cnt = (_CNT_t *)thread_block;
	client_data_t * queue_item = asyncqueue_pop(cnt->tkr_queues_to_client[TKR_2_ME_RECEIVE_MASTER_FT]);
	FileTable * ft = NULL;
	if (queue_item != NULL) {

		// already deserialized
		ft = (FileTable *)queue_item->data;
		*length_deserialized = queue_item->data_len;

		//free(queue_item->data);
		free(queue_item);
	}

	return ft;
}

// receive request for chunk : CLT_2_ME_REQUEST_CHUNK
/* poll the network to see if we have received a chunk request
 * from a peer client 
 * 	cnt - the current state of the network
 * 	filepath - an unallocated pointer to a filepath for the chunky file
 *	peer_id - the peer that made the request 
 * 	buf - the chunk that was requested
 * 		TODO - figure out how to combine this with chunky file and
 *		how to expand on it to allow for transerring an entire file
 */
int receive_chunk_request(CNT * thread_block, int *peer_id, char **filepath, int *chunk_id){
	if (!thread_block || !filepath || !peer_id || !chunk_id) {
		format_printf(err_format, "receive_chunk_req failed due to null args\n");
		return -1;
	}

	int rc = -1;

	_CNT_t * cnt = (_CNT_t *)thread_block;
	chunk_data_t * queue_item = asyncqueue_pop(cnt->clt_queues_to_client[CLT_2_ME_REQUEST_CHUNK]);
	if (queue_item != NULL) {

		*peer_id = queue_item->client_id;
		*chunk_id = queue_item->chunk_num;
		*filepath = queue_item->file_name;
		rc = 1;

		// free(queue_item->file_name)	-> just pass the buffer to client
		free(queue_item);
	}

	return rc;
}

// receive chunk from peer : CLT_2_ME_RECEIVE_CHUNK
/* client calls this to receive a chunk update from the network 
 * 	cnt - (not claimed) thread block
 * 	peer_id - (not claimed) will be filled with responder id
 * 	file_name - (not claimed) will be filled with filename
 * 	chunk_id 	- (not claimed) will be filled with chunk id
 *	data_len - (not claimed) will be filled with length of chunk data 
 *
 *	Note!!! If recieve_chunk returns 1 and data_len == -1, then that's a rejection response!
 *
 * 	data - (not claimed) will be filled with pointer to data
 * 	ret : (static) 1 if chunk was received, -1 if no chunk
 */
int receive_chunk(CNT * thread_block, int *peer_id, char **file_name, int *chunk_id, int *data_len, char **data) {
	if (!thread_block || !peer_id || !file_name || !chunk_id || !data_len || !data) {
		format_printf(err_format, "receive_chunk error: null arguments\n");
		return -1;
	}

	_CNT_t * cnt = (_CNT_t *)thread_block;
	chunk_data_t * queue_item = (chunk_data_t*)asyncqueue_pop(cnt->clt_queues_to_client[CLT_2_ME_RECEIVE_CHUNK]);
	if (queue_item != NULL) {
		*peer_id = queue_item->client_id;
		*file_name = queue_item->file_name;
		*chunk_id = queue_item->chunk_num;
		*data_len = queue_item->data_len;
		*data = queue_item->data;

		// don't free file str or data buffer -> pass to logic
		free(queue_item);
	}

	return (queue_item != NULL) ? 1 : -1;
}


/* ----- sending ----- */

// send current status : ME_2_TKR_CUR_STATUS
// 	thread : (not claimed) thread block
// 	fs : (not claimed) filesystem to send
// 	ret : (static) 1 on success, -1 on failure
int send_status(CNT * thread, FileSystem * fs) {
	if (!thread || !fs)
		return -1;
	
	_CNT_t * cnt = (_CNT_t *)thread;

	client_data_t * queue_item = malloc(sizeof(client_data_t));
	filesystem_serialize(fs, (char **)&queue_item->data, &queue_item->data_len);
	asyncqueue_push(cnt->tkr_queues_from_client[ME_2_TKR_CUR_STATUS], (void *)queue_item);
	return 1;
}

// send file acquistion update : ME_2_TKR_ACQ_UPDATE
// send current status : ME_2_TKR_CUR_STATUS
// 	thread : (not claimed) thread block
// 	path : (not claimed) filepath for file updated
// 	chunkNum : (not claimed) the chunk number that client receives
// 	ret : (static) 1 on success, -1 on failure
int send_chunk_got(CNT * thread, char * path, int chunkNum) {
	if (!thread || !path) {
		return -1;
	}
	
	_CNT_t * cnt = (_CNT_t *)thread;

	client_data_t * queue_item = malloc(sizeof(client_data_t));

	queue_item->data_len = strlen(path) + 1 + sizeof(int);
	queue_item->data = (void *)malloc(queue_item->data_len);
	memcpy(queue_item->data, &chunkNum, sizeof(int));
	memcpy( (char*)((long)queue_item->data + (long)sizeof(int)), path, queue_item->data_len - sizeof(int));

	asyncqueue_push(cnt->tkr_queues_from_client[ME_2_TKR_ACQ_UPDATE], (void *)queue_item);
	return 1;
}

// send updated file message to tracker : ME_2_TKR_UPDATED_FILE_DIFF
// 	thread : (not claimed) thread block
//	additions : (not claimed) addition FS - claim after call
// 	deletions : (not claimed) deletions FS - claim after call
//	source_originator_id : (not claimed) will hold diff's author id
//	ret : 1 on success, -1 on failure
int send_updated_files(CNT * thread, FileSystem * additions, FileSystem * deletions) {
	if (!thread || !additions || !deletions)
		return -1;

	_CNT_t * cnt = (_CNT_t *)thread;
	client_data_t * queue_item = malloc(sizeof(client_data_t));

	int length1 = -1, length2 = -1;
	char * buf1, * buf2;
	filesystem_serialize(additions, &buf1, &length1);
	filesystem_serialize(deletions, &buf2, &length2);
	if (!buf1 || !buf2) {
		format_printf(err_format,"send_updated_files error during serialization\n");
		return -1;
	}

	queue_item->data = malloc(length1 + length2);
	memcpy(queue_item->data, buf1, length1);
	memcpy( ((char *)queue_item->data) + length1, buf2, length2);
	queue_item->data_len = length1 + length2;

	free(buf1);
	free(buf2);

	asyncqueue_push(cnt->tkr_queues_from_client[ME_2_TKR_UPDATED_FILE_DIFF], (void *)queue_item);
	return 1;
}

// send request for master JFS to tracker : ME_2_TKR_GET_MASTER
// returns 1 on success, -1 on failure
int send_request_for_master(CNT * cnt) {
	_CNT_t * thread_block = (_CNT_t *) cnt;
	asyncqueue_push(thread_block->tkr_queues_from_client[ME_2_TKR_GET_MASTER], (void*)(long)1);
	return 1;
}

// send request for chunk to peer : ME_2_CLT_REQ_CHUNK
/* send a chunk request from the client app to a peer client 
 * 	cnt - the current state of the network
 * 	filepath - (not claimed) a pointer to a filepath for the chunky file
 *	peer_id - the peer to request the chunk from 
 * 	buf - the chunk that we are requesting
 * 		TODO - figure out how to combine this with chunky file and
 * 		how to expand on it to allow for transerring an entire file
 */
int send_chunk_request(CNT * thread_block, int peer_id, char *filepath, int chunk_id) {
	if (!thread_block || !filepath) {
		format_printf(err_format, "send_chunk_request error: null cnt or filepath\n");
		return -1;
	} 

	_CNT_t * cnt = (_CNT_t *)thread_block;

	if (!get_peer_by_id(cnt->peer_table, peer_id)) {
		format_printf(err_format, "send_chunk_request error: can't find peer %d\n",peer_id);
		return -1;
	}

	chunk_data_t * queue_item = (chunk_data_t *)malloc(sizeof(chunk_data_t));
	queue_item->client_id = peer_id;
	queue_item->chunk_num = chunk_id;
	queue_item->file_name = strdup(filepath);
	queue_item->file_str_len = strlen(queue_item->file_name) + 1;	// include null terminator
	queue_item->data_len = 0;
	queue_item->data = 0;

	asyncqueue_push(cnt->clt_queues_from_client[ME_2_CLT_REQ_CHUNK], (void *)queue_item);
	return 1;
}

// send chunk to client : ME_2_CLT_SEND_CHUNK
/* client calls this to send a chunk update to another client 
 * 	cnt - current state of the network 
 * 	peer_id - the peer that we are sending it to
 *	(claimed) buf	- the chunk that we are sending
 * 	len - the length of the chunk that we are sending 
 */
int send_chunk(CNT * thread_block, int peer_id, char * file_name, int chunk_id, char *chunk_data, int chunk_len) {
	if (!thread_block || !file_name || !chunk_data) {
		format_printf(err_format, "send_chunk error: null arguments\n");
		return -1;
	}

	_CNT_t * cnt = (_CNT_t *)thread_block;

	if (!get_peer_by_id(cnt->peer_table, peer_id)) {
		format_printf(err_format, "send_chunk error: can't find peer %d\n",peer_id);
		return -1;
	}

	chunk_data_t * queue_item = (chunk_data_t *)malloc(sizeof(chunk_data_t));
	queue_item->client_id = peer_id;
	queue_item->chunk_num = chunk_id;
	queue_item->file_str_len = strlen(file_name) + 1;
	queue_item->file_name = strdup(file_name);
	queue_item->data_len = chunk_len;

	// make data copy
	queue_item->data = (char *)malloc(chunk_len); 	
	memcpy(queue_item->data, chunk_data, chunk_len);

	asyncqueue_push(cnt->clt_queues_from_client[ME_2_CLT_SEND_CHUNK], (void*)queue_item);
	return 1;
}


// send chunk request error response : ME_2_CLT_SEND_ERROR
int send_chunk_rejection(CNT * thread_block, int peer_id, char *filepath, int chunk_id) {
	if (!thread_block || !filepath) {
		format_printf(err_format, "send_chunk_rejection error: null arguments\n");
		return -1;
	}

	_CNT_t * cnt = (_CNT_t *)thread_block;

	if (!get_peer_by_id(cnt->peer_table, peer_id)) {
		format_printf(err_format, "send_chunk_rejection error: cannot find peer %d\n", peer_id);
		return -1;
	}

	chunk_data_t * queue_item = (chunk_data_t *)malloc(sizeof(chunk_data_t));
	queue_item->client_id = peer_id;
	queue_item->chunk_num = chunk_id;
	queue_item->file_str_len = strlen(filepath) + 1; 	// include null terminator
	queue_item->file_name = strdup(filepath);
	queue_item->data_len = -1;
	queue_item->data = NULL;

	asyncqueue_push(cnt->clt_queues_from_client[ME_2_CLT_SEND_ERROR], (void*)queue_item);
	return 1;
}


/* ###################### *
 * 
 * Things that the network can do to interact with the client
 *
 * ###################### */

// puts fs update update on queue : TKR_2_ME_FS_UPDATE
int notify_fs_update(_CNT_t * cnt, client_data_t * queue_item) {
	if (!cnt || !queue_item)
		return -1;

	asyncqueue_push(cnt->tkr_queues_to_client[TKR_2_ME_FS_UPDATE], queue_item);
	return -1;
}

// pushes received master onto queue
//	returns 1 on success, -1 on failure
int notify_master_received(_CNT_t * cnt, client_data_t * queue_item) {
	if (!cnt || !queue_item)
		return -1;

	asyncqueue_push(cnt->tkr_queues_to_client[TKR_2_ME_RECEIVE_MASTER_JFS], (void *)queue_item);
	return 1;
}

int notify_file_acq_update(_CNT_t * cnt, client_data_t * queue_item) {
	if (!cnt || !queue_item) {
		return -1;
	}

	asyncqueue_push(cnt->tkr_queues_to_client[TKR_2_ME_FILE_ACQ], (void *)queue_item);
	return 1;
}

int notify_peer_added(_CNT_t * cnt, int added_client_id) {
	if (!cnt)
		return -1;

	asyncqueue_push(cnt->tkr_queues_to_client[TKR_2_ME_PEER_ADDED], (void *)(long)added_client_id);
	return 1;
}

int notify_peer_removed(_CNT_t * cnt, int removed_client_id) {
	if (!cnt)
		return -1;

	asyncqueue_push(cnt->tkr_queues_to_client[TKR_2_ME_PEER_DELETED], (void *)(long)(removed_client_id));
	return 1;
}

int notify_chunk_request(_CNT_t * cnt, chunk_data_t * queue_item) {
	if (!cnt || !queue_item)
		return -1;

	asyncqueue_push(cnt->clt_queues_to_client[CLT_2_ME_REQUEST_CHUNK],(void*)queue_item);
	return 1;
}

int notify_chunk_received(_CNT_t * cnt, chunk_data_t * queue_item) {
	if (!cnt || !queue_item)
		return -1;

	asyncqueue_push(cnt->clt_queues_to_client[CLT_2_ME_RECEIVE_CHUNK], (void*)queue_item);
	return 1;
}

int notify_error_received(_CNT_t * cnt, chunk_data_t * queue_item) {
	if (!cnt || !queue_item)
		return -1;

	asyncqueue_push(cnt->clt_queues_to_client[CLT_2_ME_RECEIVE_CHUNK], (void*)queue_item);
	return 1;
}

int notify_master_ft_received(_CNT_t * cnt, client_data_t * queue_item) {
	if (!cnt || !queue_item) {
		return -1;
	}

	asyncqueue_push(cnt->tkr_queues_to_client[TKR_2_ME_RECEIVE_MASTER_FT], (void*)queue_item);
	return 1;
}

/* ------------------------ FUNCTION DECLARATIONS ------------------------ */

/* --- Network Side Packet Functions --- */

// returns tracker socket (or -1 on failure)
int connect_to_tracker(char * ip_addr, int ip_len);

// returns listening port for client connections
int open_peer_listening_port();
int connect_to_peer(peer_t * peer, int id);
int disconnect_from_peer(peer_t * peer, int id);

// both paranoidly return the incremented length 
int increment_conn_record(_CNT_t * cnt, int client_id);
int decrement_conn_record(_CNT_t * cnt, int client_id);

int handle_tracker_msg(_CNT_t * cnt);
int handle_peer_msg(int sockfd, _CNT_t * cnt);

int send_tracker_message(_CNT_t * cnt, client_data_t * data_item, client_to_tracker_t type);
int send_peer_message(_CNT_t * cnt, int peer_id);

void send_heartbeat(_CNT_t * cnt);

/* --- Network Side Queue functions --- */
void poll_queues(_CNT_t * cnt);
void check_cur_status_q(_CNT_t * cnt);
void check_file_acq_q(_CNT_t * cnt);
void check_updated_fs_q(_CNT_t * cnt);
//void check_quit_q(_CNT_t * cnt);
void check_master_req_q(_CNT_t * cnt);
void check_req_chunk_q(_CNT_t * cnt);
void check_send_chunk_q(_CNT_t * cnt);
void check_req_error_q(_CNT_t * cnt);

/* ------------------------ NETWORK THREAD ------------------------ */

void * clt_network_start(void * arg) {

	_CNT_t * cnt = (_CNT_t *)arg;
	if (!cnt) {
		format_printf(err_format, "cannot start client network thread because argument is null\n");
		return (void *)-1;
	}

	while ((cnt->tracker_fd = connect_to_tracker(cnt->ip_addr, cnt->ip_len)) < 0) {
		format_printf(err_format, "Failed to connect with tracker...\n");
		sleep(5);
	}

	/* need to open peer listening socket */
	cnt->peer_listening_fd = open_peer_listening_port();
	if (cnt->peer_listening_fd < 0) {
		format_printf(err_format, "failed to open peer listening socket\n");
		return (void *)-1;
	}

	listen(cnt->peer_listening_fd, MAX_CLIENT_QUEUE);

	// for connecting with new clients
	struct sockaddr_in clientaddr;
	unsigned int addrlen = sizeof(struct sockaddr_in);

	// set up timer
	struct timeval timeout;
	timeout.tv_sec = NETWORK_WAIT;
	timeout.tv_usec = 0;

	// select fd sets
	fd_set read_fd_set, active_fd_set;
	FD_ZERO(&active_fd_set);

	// add listening socket to fd set
	FD_SET(cnt->tracker_fd, &active_fd_set);
	FD_SET(cnt->peer_listening_fd, &active_fd_set);

	time_t last_heartbeat = 0, current_time;
	int connected = 1;
	while (connected) {

		read_fd_set = active_fd_set;
		if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, &timeout) < 0) {
			format_printf(err_format, "network client failed to select amongst inputs\n");
			connected = 0;
			continue;
		}

		//printf("\nclient network polling connections...\n");
		for (int i = 0; i < FD_SETSIZE; i++) {
			if (FD_ISSET(i, &read_fd_set)) {

				/* -- from tracker -- */
				if (i == cnt->tracker_fd) {

					format_printf(network_format,"\nclient network received message from tracker (socket %d)\n", i);
					if (handle_tracker_msg(cnt) != 1) {
						fprintf(stderr, "failed to handle tracker message. Ending.\n");
						connected = 0;
						continue;
					}

				/* -- new peer connection -- */
				} else if (i == cnt->peer_listening_fd) {

					int new_peer_fd = accept(cnt->peer_listening_fd, (struct sockaddr *)&clientaddr, &addrlen);
					if (new_peer_fd < 0) {
						format_printf(err_format,"network client failed to accept new client connection\n");
						continue;
					}

					format_printf(network_format,"\nclient network received new connection from peer at %s (socket %d)\n",inet_ntoa(clientaddr.sin_addr),i);
					peer_t * new_peer = get_peer_by_ip(cnt->peer_table, (char *)&clientaddr.sin_addr);
					if (!new_peer) {
						format_printf(err_format, "client network doesn't has a peer record for new peer\n");
						close(new_peer_fd);
						continue;
					}

					// record peer connection socket and add to active set of sockets
					new_peer->socketfd = new_peer_fd;
					FD_SET(new_peer->socketfd, &active_fd_set);

				/* -- else peer message -- */
				} else {

					format_printf(network_format,"\nclient network received message from peer on socket %d\n", i);
					if (handle_peer_msg(i, cnt) != 1) {
						format_printf(err_format, "Error receiving from socket %d -- Ending session\n", i);

						// clean up existing peer connection data
						peer_t * dead_peer = get_peer_by_socket(cnt->peer_table, i);
						if (dead_peer != NULL) {
							dead_peer->socketfd = -1;

							conn_rec_t dead_record;
							dead_record.client_id = dead_peer->id;
							conn_rec_t * request_record = (conn_rec_t *)hashtable_get_element(cnt->request_table, &dead_record);
							if (request_record) {
								request_record->outstanding_requests = 0;
							}
						}
						
						FD_CLR(i, &active_fd_set);
						close(i);
					}
				}
			}
		} 	// end of socket scan

		/* send heart beat if necessary */
		current_time = time(NULL);
		if (current_time - last_heartbeat > (DIASTOLE)) {
			send_tracker_message(cnt, NULL, HEARTBEAT);
			last_heartbeat = current_time;
		}

		/* poll queues for messages from client logic */
		poll_queues(cnt);

	}

	return (void *)1;
}

/* ------------------------ HANDLE PACKETS ON NETWORK ------------------------ */

// connects to the tracker
//	returns connected socket if successful
// 	else returns -1
int connect_to_tracker(char * ip_addr, int ip_len) {
	if (!ip_addr) {
		format_printf(err_format, "failed to connect to tracker: null ip\n");
		return -1;
	}

	// int sockfd;
	// if ((sockfd = socket(AF_INET6, SOCK_STREAM, 6)) < 0) {
	// 	perror("client_network thread failed to create tracker socket");
	// 	return -1;
	// }
	int sockfd;
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 6)) < 0) {
		perror("client_network thread failed to create tracker socket");
		return -1;
	}

	// fill out server struct
	// struct sockaddr_in6 servaddr;
	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	memcpy(&servaddr.sin_addr, ip_addr, ip_len);
	//inet_pton(AF_INET, ip_addr, &servaddr.sin_addr);
	// servaddr.sin_addr = inet_addr(ip_addr);
	servaddr.sin_port = htons(TRACKER_LISTENING_PORT);

	// memcpy(&servaddr.sin6_addr, ip_addr, ip_len);
	// memcpy(&servaddr.sin_addr, ip_addr, ip_len);
	// servaddr.sin6_family = AF_INET6;
	// servaddr.sin6_port = htons(TRACKER_LISTENING_PORT); 

	// connect
	if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
		perror("client_network thread failed to connect to tracker");
		return -1;
	}

	// char ip_str[INET6_ADDRSTRLEN] = "";
	char* ip_str = inet_ntoa(servaddr.sin_addr);
	// if (strcmp(ip_str, "")!= 0){
	// 	perror("inet_ntoa failed");
	// 	return -1;
	// }

	format_printf(network_format,"NETWORK connected to tracker at %s on port %d (socket %d)\n",ip_str,TRACKER_LISTENING_PORT,sockfd);
	//printf("NETWORK connected to tracker at %s on port %d (socket %d)\n", ip_str,TRACKER_LISTENING_PORT,sockfd);
	return sockfd;
}

int open_peer_listening_port() {

	// create a socket descriptor
  int sockfd;
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 6)) < 0) {
      perror("peer listening socket creation -- error creating listening socket");
      return -1;
  }

  // eliminate "Address already in use" error from bind.
  int opt_yes = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt_yes, sizeof(int)) == -1) {
      perror("peer listening socket creation -- setsockopt failure");
      return -1;
  } 

  struct sockaddr_in server_addr;
  bzero(&server_addr, sizeof(struct sockaddr_in));
  server_addr.sin_family = AF_INET;               // IPv4
  server_addr.sin_addr.s_addr = INADDR_ANY;       // use any IPs of machine
  server_addr.sin_port = htons((uint16_t)CLIENT_LISTENING_PORT);

  if (bind(sockfd, (struct sockaddr *) &server_addr, sizeof(struct sockaddr)) != 0) {
      perror("peer listening socket creation -- error binding");
      return -1;
  }

  format_printf(network_format,"NETWORK -- client listening for peer connections at %s on port %d\n", inet_ntoa(server_addr.sin_addr), CLIENT_LISTENING_PORT);
  // make it a listening socket ready to accept connection requests
  return(sockfd);
}

// returns connected socket for peer (also in the peer struct)
int connect_to_peer(peer_t * peer, int id) {
	if (!peer || id < 1) {
		format_printf(err_format, "cannot connect to null peer %d\n", id);
		return -1;
	}

	if (peer->socketfd > 0)
		return peer->socketfd;

	int sockfd = socket(AF_INET, SOCK_STREAM, 6);
	if (sockfd < 1) {
		perror("failed to open peer 2 peer socket");
		return -1;
	}

	// fill out server struct
	// struct sockaddr_in6 servaddr;
	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	memcpy(&servaddr.sin_addr, peer->ip_addr, sizeof(servaddr.sin_addr));
	servaddr.sin_port = htons(CLIENT_LISTENING_PORT);

	// connect
	if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
		format_printf(err_format, "client_network thread failed to connect to peer %d at %s", id, inet_ntoa(*(struct in_addr*)&peer->ip_addr));
		perror("reason:");
		return -1;
	}

	peer->socketfd = sockfd;

	format_printf(network_format,"NETWORK -- connected to peer %d at %s on socket %d\n", id, inet_ntoa(*(struct in_addr*)&peer->ip_addr), sockfd);
	return sockfd;
}

int disconnect_from_peer(peer_t * peer, int id) {
	if (!peer)
		return -1;

	if (peer->socketfd > 0) {
		format_printf(network_format,"NETWORK -- closing connection to peer %d at %s\n", id, inet_ntoa(*(struct in_addr *)&peer->ip_addr));
		close(peer->socketfd);
		peer->socketfd = -1;
	}

	return 1;
}

// both paranoidly return the incremented length 
int increment_conn_record(_CNT_t * cnt, int client_id) {
	if (!cnt) {
		return -1;
	}

	conn_rec_t record;
	record.client_id = client_id;
	conn_rec_t * entry = hashtable_get_element(cnt->request_table, &record);
	if (!entry) {
		// make a crecord 
		conn_rec_t * new_record = (conn_rec_t *)malloc(sizeof(conn_rec_t));
		new_record->client_id = client_id;
		new_record->outstanding_requests = 0;
		hashtable_add(cnt->request_table, (void*)new_record);
		entry = new_record;
	}

	return (entry != NULL) ? ++entry->outstanding_requests : -1;
}
int decrement_conn_record(_CNT_t * cnt, int client_id) {
	if (!cnt) {
		return -1;
	}

	conn_rec_t record;
	record.client_id = client_id;
	conn_rec_t * entry = hashtable_get_element(cnt->request_table, &record);

	return (entry != NULL) ? --entry->outstanding_requests : -1;
}

int handle_tracker_msg(_CNT_t * cnt) {
	if (!cnt)
		return -1;

	tracker_pkt_t pkt;
	if (recv(cnt->tracker_fd, &pkt, sizeof(tracker_pkt_t), 0) != sizeof(tracker_pkt_t)){
		format_printf(err_format, "client network failed to get message from tracker\n");
		return -1;
	}

	char * buf = NULL;
	if (pkt.data_len > 0) {
		buf = calloc(1,pkt.data_len);
		if (recv(cnt->tracker_fd, buf, pkt.data_len, 0) != pkt.data_len) {
			format_printf(err_format, "client network failed to get data from tracker\n");
			return -1;
		}
	}

	switch (pkt.type) {
		case TRANSACTION_UPDATE:
			format_printf(err_format,"NETWORK -- received transaction update from tracker -- unhandled\n");
			// process transaction update
			break;

		case FS_UPDATE:
			format_printf(network_format,"NETWORK -- received a file system UPDATE from tracker\n");

			client_data_t * queue_item = malloc(sizeof(client_data_t));
			queue_item->data_len = pkt.data_len;
			queue_item->data = buf;
			buf = NULL; 	// pass buff to queue and don't claim here

			notify_fs_update(cnt, queue_item);
			break;

		case FILE_ACQ_UPDATE:	
			format_printf(network_format, "NETWORK -- received a chunk acquisition update from tracker\n");

			client_data_t * file_acq_update = malloc(sizeof(client_data_t));
			file_acq_update->data_len = pkt.data_len;
			file_acq_update->data = buf;
			buf = NULL;

			notify_file_acq_update(cnt, file_acq_update);
			break;

		case MASTER_STATUS:
			format_printf(network_format,"NETWORK -- received master JFS from tracker\n");
			client_data_t * master_update = malloc(sizeof(client_data_t));	
			master_update->data = (void*)filesystem_deserialize(buf, &master_update->data_len);
			if (!master_update->data || master_update->data_len < 0) {
				fprintf(stderr, "failed to unpack master JFS\n");
				break;
			}

			notify_master_received(cnt, master_update);
			break;

		case MASTER_FT:
			format_printf(network_format,"NETWORK -- received master file table from tracker\n");
			client_data_t * master_ft = malloc(sizeof(client_data_t));
			master_ft->data = (void *)filetable_deserialize(buf, &master_ft->data_len);
			if (!master_ft->data){
				format_printf(network_format, "NETWORK -- ft deserialize returned NULL\n");
			}

			notify_master_ft_received(cnt, master_ft);
			break;

		case PEER_TABLE:
			format_printf(network_format,"NETWORK -- received peer table from tracker\n");

			peer_table_t * additions, * deletions, * new_table;
			new_table = deserialize_peer_table(buf, pkt.data_len); 	// buf is claimed
			buf = NULL; 

			if (!new_table) {
				format_printf(err_format,"client network failed to deserialize tracker's peer table\n");
				break;
			}

			diff_tables(cnt->peer_table, new_table, &additions, &deletions);

			// notify logic about additions
			for (int i = 0; i < additions->size; i++) {
				if (additions->peer_list[i] != NULL) {
					notify_peer_added(cnt, additions->peer_list[i]->id);
				}
			}

			// notify logic about deletions
			for (int i = 0; i < deletions->size; i++) {
				if (deletions->peer_list[i] != NULL) {
					notify_peer_removed(cnt, deletions->peer_list[i]->id);
				}
			}

			destroy_table(additions);
			destroy_table(deletions);
			destroy_table(cnt->peer_table);

			// replace current peer table with received peer table
			cnt->peer_table = new_table;
			format_printf(network_format,"\nnew peer table\n");
			print_table(cnt->peer_table);
			break;

		case PEER_ADDED:
			format_printf(network_format,"Received new peer. Adding to peer table\n");
			break;
		case PEER_DELETED:
			format_printf(network_format,"Deleted peer. Removing from peer table\n");
			break;
	
		default:
			format_printf(network_format,"CLIENT NETWORK received unhandled packet of type %d\n", pkt.type);
			break;
	}

	if (buf != NULL) {
		free(buf);
	}
		
	return 1;
}

int handle_peer_msg(int sockfd, _CNT_t * cnt) {

	if (!cnt)
		return -1;

	peer_t * peer = get_peer_by_socket(cnt->peer_table, sockfd);

	c2c_pkt_t pkt;
	if (recv(sockfd, &pkt, sizeof(pkt), 0) != sizeof(c2c_pkt_t)) {
		format_printf(err_format,"didn't recv full pkt on socket %d\n", sockfd);
		return -1;
	}

	char * file_name_buf = NULL;
	char * data_buf = NULL;

	if (pkt.file_str_len > 0) {
		file_name_buf = (char *)malloc(pkt.file_str_len);
		if (recv(sockfd, file_name_buf, pkt.file_str_len, 0) != pkt.file_str_len) {
			format_printf(err_format,"didn't recv full file str on socket %d\n", sockfd);
			return -1;
		}
	}

	if (pkt.data_len > 0) {
		data_buf = (char *)malloc(pkt.data_len);
		if (recv(sockfd, data_buf, pkt.data_len, 0) != pkt.data_len) {
			format_printf(err_format,"didn't recv full data length on socket %d\n", sockfd);
			return -1;
		}
	}

	chunk_data_t * queue_item = (chunk_data_t *)malloc(sizeof(chunk_data_t));

	switch (pkt.type) {
		case REQUEST_CHUNK:

			format_printf(network_format,"NETWORK -- received chunk request (%s, %d) from client %d\n", file_name_buf, pkt.chunk_num, peer->id);
			queue_item->client_id = peer->id;
			queue_item->chunk_num = pkt.chunk_num;
			queue_item->file_str_len = pkt.file_str_len;

			queue_item->file_name = file_name_buf;
			file_name_buf = NULL; 	// don't free at the end of this function

			queue_item->data_len = 0;
			queue_item->data = NULL;

			notify_chunk_request(cnt, queue_item);
			break;

		case CHUNK:

			format_printf(network_format,"NETWORK -- received chunk (%s, %d) from peer %d\n", file_name_buf, pkt.chunk_num, peer->id);
			queue_item->client_id = peer->id;
			queue_item->chunk_num = pkt.chunk_num;
			queue_item->file_str_len = pkt.file_str_len;

			queue_item->file_name = file_name_buf;
			file_name_buf = NULL; 	// don't free at the end of this function

			queue_item->data_len = pkt.data_len;
			queue_item->data = data_buf;
			data_buf = NULL; // don't free here -- pass to notify queue

			// disconnect from peer if all requests have been fulfilled
			if (decrement_conn_record(cnt, peer->id == 0)) {
				disconnect_from_peer(peer, peer->id);
			}

			notify_chunk_received(cnt, queue_item);
			break;

		case REQ_REJECT:

			format_printf(network_format,"NETWORK -- received chunk request rejection (%s, %d) from peer %d\n", file_name_buf, pkt.chunk_num, peer->id);
			queue_item->client_id = peer->id;
			queue_item->file_str_len = pkt.file_str_len;

			queue_item->file_name = file_name_buf;
			file_name_buf = NULL; // pass onto queue -- don't free here

			queue_item->chunk_num = pkt.chunk_num;
			queue_item->data_len = pkt.data_len;
			queue_item->data = NULL;

			// disconnect from peer if all requests have been fulfilled
			if (decrement_conn_record(cnt, peer->id == 0)) {
				disconnect_from_peer(peer, peer->id);
			}

			notify_error_received(cnt, queue_item);
			break;

		default:
			format_printf(network_format,"NETWORK -- handled client message received (%d)\n", pkt.type);
			break;
	}

	if (file_name_buf) {
		free(file_name_buf);
	}

	if (data_buf) {
		free(data_buf);
	}

	return 1;
}

int send_tracker_message(_CNT_t * cnt, client_data_t * data_item, client_to_tracker_t type) {
	if (!cnt)
		return -1;

	client_pkt_t pkt;

	switch (type) {
		case HEARTBEAT:
			format_printf(network_format,"NETWORK -- sending tracker heartbeat\n");
			pkt.type = HEARTBEAT;
			pkt.data_len = 0;
			if (send(cnt->tracker_fd, &pkt, sizeof(client_pkt_t), 0) != sizeof(client_pkt_t)) {
				perror("client network failed to send heartbeat");
				return -1;
			}
			break;

		default:
			format_printf(err_format,"client network cannot send unknown packet type %d to tracker\n", type);
			break;
	}

	return 1;
}

int send_peer_message(_CNT_t * cnt, int peer_id) {

	return -1;
}

/* ------------------------ HANDLE QUEUES TO NETWORK FROM LOGIC ------------------------ */

void poll_queues(_CNT_t * cnt) {
	//printf("\nclient network is polling queues\n");
	check_cur_status_q(cnt);
	check_file_acq_q(cnt);
	//check_quit_q(cnt);
	check_master_req_q(cnt);
	check_updated_fs_q(cnt);

	check_req_chunk_q(cnt);
	check_send_chunk_q(cnt);
	check_req_error_q(cnt);
}

void check_cur_status_q(_CNT_t * cnt) {
	AsyncQueue * q = cnt->tkr_queues_from_client[ME_2_TKR_CUR_STATUS];

	client_data_t * queue_item = asyncqueue_pop(q);
	while ( (queue_item = asyncqueue_pop(q)) != NULL) {

		format_printf(network_format,"NETWORK -- sending current status JFS to tracker\n");
		client_pkt_t pkt;
		pkt.type = CLIENT_STATE;
		pkt.data_len = queue_item->data_len;
		send(cnt->tracker_fd, &pkt, sizeof(client_pkt_t), 0);
		send(cnt->tracker_fd, queue_item->data, queue_item->data_len, 0);

		free(queue_item->data);
		free(queue_item);
	}

	return;
}

void check_file_acq_q(_CNT_t * cnt) {
	AsyncQueue * q = cnt->tkr_queues_from_client[ME_2_TKR_ACQ_UPDATE];
	client_data_t * queue_item = asyncqueue_pop(q);
	while ( (queue_item = asyncqueue_pop(q)) != NULL) {

		format_printf(network_format, "NETWORK -- sending file acquisition update to tracker\n");

		client_pkt_t pkt;
		pkt.type = CHUNK_GOT;
		pkt.data_len = queue_item->data_len;

		send(cnt->tracker_fd, &pkt, sizeof(pkt),0);
		send(cnt->tracker_fd, queue_item->data, queue_item->data_len, 0);

		free(queue_item->data);
		free(queue_item);
	}

	return;
}

void check_updated_fs_q(_CNT_t * cnt) {
	AsyncQueue * q = cnt->tkr_queues_from_client[ME_2_TKR_UPDATED_FILE_DIFF];
	
	client_data_t * queue_item = asyncqueue_pop(q);
	while ( (queue_item = asyncqueue_pop(q)) != NULL) {
		format_printf(network_format,"NETWORK -- sending updated file difference\n");
		client_pkt_t pkt;
		pkt.type = CLIENT_UPDATE;
		pkt.data_len = queue_item->data_len;

		send(cnt->tracker_fd, &pkt, sizeof(client_pkt_t), 0);
		send(cnt->tracker_fd, queue_item->data, queue_item->data_len, 0);

		free(queue_item->data);
		free(queue_item);
	}	
	return;
}

void check_master_req_q(_CNT_t * cnt) {
	AsyncQueue * q = cnt->tkr_queues_from_client[ME_2_TKR_GET_MASTER];
	while (1 == (int)(long)asyncqueue_pop(q)) {

		format_printf(network_format,"NETWORK -- sending request for master JFS\n");
		client_pkt_t pkt;
		pkt.type = REQUEST_MASTER;
		pkt.data_len = 0;
		send(cnt->tracker_fd, &pkt, sizeof(client_pkt_t), 0);

	}
	return;
}

void check_req_chunk_q(_CNT_t * cnt) {
	AsyncQueue * q = cnt->clt_queues_from_client[ME_2_CLT_REQ_CHUNK];
	chunk_data_t * queue_item = asyncqueue_pop(q);
	while ( (queue_item = asyncqueue_pop(q)) != NULL) {

		// open connection to peer if not already open
		peer_t * peer = get_peer_by_id(cnt->peer_table, queue_item->client_id);
		if (connect_to_peer(peer, queue_item->client_id) < 0) {
			format_printf(err_format,"network client failed to send connect to client %d\n",queue_item->client_id);
			return;
		}

		c2c_pkt_t pkt;
		pkt.type = REQUEST_CHUNK;
		pkt.chunk_num = queue_item->chunk_num;
		pkt.file_str_len = queue_item->file_str_len;
		pkt.data_len = queue_item->data_len;

		send(peer->socketfd, &pkt, sizeof(pkt),0);
		send(peer->socketfd, queue_item->file_name, pkt.file_str_len,0);

		format_printf(network_format,"NETWORK -- sent request for chunk %s %d to client %d\n", queue_item->file_name, pkt.chunk_num, peer->id);
		increment_conn_record(cnt, peer->id);

		free(queue_item->file_name);
		free(queue_item);
	}
	return;
}

void check_send_chunk_q(_CNT_t * cnt) {
	AsyncQueue * q = cnt->clt_queues_from_client[ME_2_CLT_SEND_CHUNK];
	chunk_data_t * queue_item = asyncqueue_pop(q);

	while ( (queue_item = asyncqueue_pop(q)) != NULL) {

		format_printf(network_format,"NETWORK -- sending chunk (%s, %d) to client %d\n", 
			queue_item->file_name, queue_item->chunk_num, queue_item->client_id);

		peer_t * peer = get_peer_by_id(cnt->peer_table, queue_item->client_id);
		if (!peer) {
			format_printf(err_format,"check_send_chunk_q error: cannot find peer %d\n", queue_item->client_id);
			return;
		} else if (peer->socketfd < 0) {
			format_printf(err_format,"check_send_chunk_q error: connection is not open with peer %d\n", queue_item->client_id);
			return;
		}

		c2c_pkt_t pkt;
		pkt.type = CHUNK;
		pkt.chunk_num = queue_item->chunk_num;
		pkt.file_str_len = queue_item->file_str_len;
		pkt.data_len = queue_item->data_len;

		send(peer->socketfd, &pkt, sizeof(pkt), 0);
		send(peer->socketfd, queue_item->file_name, pkt.file_str_len, 0);
		send(peer->socketfd, queue_item->data, pkt.data_len, 0);

		free(queue_item->file_name);
		free(queue_item->data);
		free(queue_item);
	}

	return;
}

void check_req_error_q(_CNT_t * cnt) {
	AsyncQueue * q = cnt->clt_queues_from_client[ME_2_CLT_SEND_ERROR];
	chunk_data_t * queue_item = asyncqueue_pop(q);
	while ( (queue_item = asyncqueue_pop(q)) != NULL) {

		format_printf(network_format,"NETWORK -- sending error response for chunk (%s, %d) to client %d\n", 
			queue_item->file_name, queue_item->chunk_num, queue_item->client_id);

		peer_t * peer = get_peer_by_id(cnt->peer_table, queue_item->client_id);
		if (!peer) {
			format_printf(err_format,"send chunk error response: cannot find peer %d\n", queue_item->client_id);
			return;
		}

		c2c_pkt_t pkt;
		pkt.type = REQ_REJECT;
		//pkt.src_client = queue_item->client_id;
		pkt.chunk_num = queue_item->chunk_num;
		pkt.file_str_len = queue_item->file_str_len;
		pkt.data_len = queue_item->data_len;

		send(peer->socketfd, &pkt, sizeof(pkt), 0);
		send(peer->socketfd, queue_item->file_name, pkt.file_str_len, 0);

		free(queue_item->file_name);
		free(queue_item);
	}
	return;
}
