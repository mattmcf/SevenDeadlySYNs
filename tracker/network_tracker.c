/*
 * network_tracker.c
 *
 * contains functionality for the tracker's network thread
 */

#include <sys/types.h>		// for networking
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h> 		// memory and IO
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <signal.h>
#include <sys/select.h> 	// for select call
#include <netinet/in.h> 	// in_addr_t struct
#include <arpa/inet.h>
#include <assert.h>
#include <sys/utsname.h>
#include <pthread.h>
#include <unistd.h>

#include "network_tracker.h"
#include "../utility/AsyncQueue/AsyncQueue.h"
#include "../common/constant.h"
#include "../common/peer_table.h"
#include "../common/packets.h"
#include "../utility/FileTable/FileTable.h"

#define INIT_CLIENT_NUM 10


/* --- 	queue definitions --- */
#define CLT_2_TKR_CUR_STATE 0
#define CLT_2_TKR_FILE_UPDATE 1
#define CLT_2_TKR_NEW_CLIENT 2
#define CLT_2_TKR_REMOVE_CLIENT 3
#define CLT_2_TKR_CLIENT_GOT 4
#define CLT_2_TKR_CLIENT_GET_FAILED 5
#define CLT_2_TKR_CLIENT_REQ_MASTER 6

#define TKR_2_CLT_TXN_UPDATE 0
#define TKR_2_CLT_FILE_ACQ 1
#define TKR_2_CLT_FS_UPDATE 2
#define TKR_2_CLT_ADD_PEER 3
#define TKR_2_CLT_REMOVE_PEER 4
#define TKR_2_CLT_SEND_MASTER 5
#define TKR_2_CLT_SEND_MASTER_FT 6

typedef struct _TNT {

	/* 
	 * *** INCOMING QUEUES ****
	 *
	 * -- client to tracker --
	 * queues_to_tracker[0] -> contains client's current JFS (to logic)
 	 * queues_to_tracker[1] -> client has just updated a file (to logic)
 	 * queues_to_tracker[2] -> client has joined network (to logic)
 	 * queues_to_tracker[3] -> client disconnected queue (to logic)
 	 * queues_to_tracker[4] -> client successful get queue (to logic)
 	 * queues_to_tracker[5] -> client failed to get queue (to logic)
 	 * queues_to_tracker[6] -> client requests for master file system
 	 *
 	 */
 	AsyncQueue ** queues_to_tracker; 		// to logic

 	/*
 	 * *** OUGOING QUEUES ***
 	 *
 	 * -- tracker to client --
 	 * queues_from_tracker[0] -> transaction update queue
 	 * queues_from_tracker[1] -> file acquisition status update queue (get and failed to get)
 	 * queues_from_tracker[2] -> file system update (client changed file)
 	 * queues_from_tracker[3] -> peer added messages to distribute
 	 * queues_from_tracker[4] -> peer removed messages to distribute
 	 * queues_from_tracker[5] -> send master file system to client
 	 * queues_from_tracker[6] -> send master file table to client
 	 *
 	 */
 	AsyncQueue ** queues_from_tracker;  	// from logic

 	/*
 	 * networking monitoring information
 	 */
 	pthread_t thread_id;
 	int listening_sockfd;
 	peer_table_t * peer_table;

} _TNT_t;

// launches network thread
void * tkr_network_start(void * arg);

TNT * StartTrackerNetwork() {

	_TNT_t * tracker_thread = (_TNT_t *)calloc(1,sizeof(_TNT_t));

	/* -- incoming from client to tracker -- */
	tracker_thread->queues_to_tracker = (AsyncQueue **)calloc(7,sizeof(AsyncQueue *));
	tracker_thread->queues_to_tracker[CLT_2_TKR_CUR_STATE] = asyncqueue_new();
	tracker_thread->queues_to_tracker[CLT_2_TKR_FILE_UPDATE] = asyncqueue_new();
	tracker_thread->queues_to_tracker[CLT_2_TKR_NEW_CLIENT] = asyncqueue_new();
	tracker_thread->queues_to_tracker[CLT_2_TKR_REMOVE_CLIENT] = asyncqueue_new();
	tracker_thread->queues_to_tracker[CLT_2_TKR_CLIENT_GOT] = asyncqueue_new();
	tracker_thread->queues_to_tracker[CLT_2_TKR_CLIENT_GET_FAILED] = asyncqueue_new();
	tracker_thread->queues_to_tracker[CLT_2_TKR_CLIENT_REQ_MASTER] = asyncqueue_new();

	/* -- outgoing from tracker to client -- */
	tracker_thread->queues_from_tracker = (AsyncQueue **)calloc(7,sizeof(AsyncQueue *));
	tracker_thread->queues_from_tracker[TKR_2_CLT_TXN_UPDATE] = asyncqueue_new();
	tracker_thread->queues_from_tracker[TKR_2_CLT_FILE_ACQ] = asyncqueue_new();
	tracker_thread->queues_from_tracker[TKR_2_CLT_FS_UPDATE] = asyncqueue_new();
	tracker_thread->queues_from_tracker[TKR_2_CLT_ADD_PEER] = asyncqueue_new();
	tracker_thread->queues_from_tracker[TKR_2_CLT_REMOVE_PEER] = asyncqueue_new();
	tracker_thread->queues_from_tracker[TKR_2_CLT_SEND_MASTER] = asyncqueue_new();
	tracker_thread->queues_from_tracker[TKR_2_CLT_SEND_MASTER_FT] = asyncqueue_new();

	/* -- set up auxillary information -- */
	tracker_thread->peer_table = init_peer_table(INIT_CLIENT_NUM);
	if (!tracker_thread->peer_table) {
		fprintf(stderr,"StartTrackerNetwork failed to create peer table\n");
		return NULL;
	}

	/* -- spin off network thread -- */
	pthread_create(&tracker_thread->thread_id, NULL, tkr_network_start, tracker_thread);

	return (TNT *)tracker_thread;
}

void EndTrackerNetwork(TNT * thread_block) {

	if (!thread_block)
		return;

	_TNT_t * tnt = (_TNT_t *)thread_block;

	asyncqueue_destroy(tnt->queues_to_tracker[CLT_2_TKR_CUR_STATE]);
	asyncqueue_destroy(tnt->queues_to_tracker[CLT_2_TKR_FILE_UPDATE]);
	asyncqueue_destroy(tnt->queues_to_tracker[CLT_2_TKR_NEW_CLIENT]);
	asyncqueue_destroy(tnt->queues_to_tracker[CLT_2_TKR_REMOVE_CLIENT]);
	asyncqueue_destroy(tnt->queues_to_tracker[CLT_2_TKR_CLIENT_GOT]);
	asyncqueue_destroy(tnt->queues_to_tracker[CLT_2_TKR_CLIENT_GET_FAILED]);
	asyncqueue_destroy(tnt->queues_to_tracker[CLT_2_TKR_CLIENT_REQ_MASTER]);
	free(tnt->queues_to_tracker);

	asyncqueue_destroy(tnt->queues_from_tracker[TKR_2_CLT_TXN_UPDATE]);
	asyncqueue_destroy(tnt->queues_from_tracker[TKR_2_CLT_FILE_ACQ]);
	asyncqueue_destroy(tnt->queues_from_tracker[TKR_2_CLT_FS_UPDATE]);
	asyncqueue_destroy(tnt->queues_from_tracker[TKR_2_CLT_ADD_PEER]);
	asyncqueue_destroy(tnt->queues_from_tracker[TKR_2_CLT_REMOVE_PEER]);
	asyncqueue_destroy(tnt->queues_from_tracker[TKR_2_CLT_SEND_MASTER]);
	asyncqueue_destroy(tnt->queues_from_tracker[TKR_2_CLT_SEND_MASTER_FT]);
	free(tnt->queues_from_tracker);

	close(tnt->listening_sockfd);

	// close all client connections
	for (int i = 0; i < tnt->peer_table->size; i++) {
		if (tnt->peer_table->peer_list[i] && tnt->peer_table->peer_list[i]->socketfd > 0) 
			close(tnt->peer_table->peer_list[i]->socketfd);
	}

	destroy_table(tnt->peer_table);

	free(tnt);

	printf("NETWORK -- ending network thread after cleaning up.\n");
	
	return;
}

/* ###################### *
 * 
 * Things that the tracker can do to interact with network
 *
 * ###################### */

/* ----- receiving ----- */

// Receives a "current state" fs message from client : CLT_2_TKR_CUR_STATE
//	fs : (not claimed) pointer that will reference deserialized client JFS -- need to be claimed by logic
// 	clientid : (not claimed) pointer to int that will be filled with client id if update is present
// 	ret : (static) 1 is success, -1 is failure (communications broke) 
int receive_client_state(TNT * tnt, FileSystem ** fs, int * clientid) {
	_TNT_t * thread_block = (_TNT_t *)tnt;
	client_data_t * data_pkt = (client_data_t*)asyncqueue_pop(thread_block->queues_to_tracker[CLT_2_TKR_CUR_STATE]);

	if (data_pkt != NULL) {

		*fs = data_pkt->data;
		*clientid = data_pkt->client_id;
		free(data_pkt);
		return 1;

	} 

	return -1;
}
// sending file acquisition: first 4 bytes (int) is chunk number, rest is filepath
// Receives a "got file chunk" message from client : CLT_2_TKR_CLIENT_GOT
//	path : (not claimed) the filepath of the acquired file
// chunkNum: (not claimed) the chunk number of the file that was acquired
// 	clientid : (not claimed) pointer to int that will be filled with client id if update is present
// 	ret : (static) 1 is success, -1 is failure (communications broke) 
int receive_client_got(TNT * tnt, char * path, int * chunkNum, int * clientid){
	_TNT_t * thread_block = (_TNT_t *)tnt;
	client_data_t * data_pkt = (client_data_t*)asyncqueue_pop(thread_block->queues_to_tracker[CLT_2_TKR_CLIENT_GOT]);
	if (data_pkt != NULL) {
		*clientid = data_pkt->client_id;
		memmove(chunkNum, data_pkt->data, sizeof(int));
		memmove(path, (char *)((long)data_pkt->data + (long)sizeof(int)), data_pkt->data_len - sizeof(int));	

		free(data_pkt->data);	
		free(data_pkt);
		return 1;
	} 
	return -1;
}

// Receive client file system update (modified file) : CLT_2_TKR_FILE_UPDATE
// 	tnt : (not claimed) thread block 
//	clientid : (not claimed) space for client id that will be filled if update is there
// 	ret : (not claimed) total number of bytes deserialized or -1 if no update available
int receive_client_update(TNT * thread, int * client_id, FileSystem ** additions, FileSystem ** deletions) {
	if (!thread || !client_id || !additions || !deletions)
		return -1;

	int additions_length = -1, deletions_length = -1;

	_TNT_t * tnt = (_TNT_t *)thread;
	client_data_t * queue_item = (client_data_t *)asyncqueue_pop(tnt->queues_to_tracker[CLT_2_TKR_FILE_UPDATE]);
	if (queue_item != NULL) {

		*client_id = queue_item->client_id;
		*additions = filesystem_deserialize((char *)queue_item->data, &additions_length);
		*deletions = filesystem_deserialize((char *)((long)queue_item->data + (long)additions_length), &deletions_length);

		free(queue_item->data);
		free(queue_item);
	}

	return (additions_length > 0) ? additions_length + deletions_length : -1;
}

// Receive notice that client joined network : CLT_2_TKR_NEW_CLIENT
// 	tnt : (not claimed) thread block
//	ret : client id (-1 if no id, id on success)
int receive_new_client(TNT * tnt) {
	_TNT_t * thread_block = (_TNT_t *) tnt;
	int id = (int)(long)asyncqueue_pop(thread_block->queues_to_tracker[CLT_2_TKR_NEW_CLIENT]);
	return (id > 0) ? id : -1;
}

// Receive notice that client should be removed from network : CLT_2_TKR_REMOVE_CLIENT
// 	tnt : (not claimed) thread block
// 	ret : client id (-1 if no id, id if client should be removed)
int receive_lost_client(TNT * tnt) {
	_TNT_t * thread_block = (_TNT_t *)tnt;
	int id = (int)(long)asyncqueue_pop(thread_block->queues_to_tracker[CLT_2_TKR_REMOVE_CLIENT]);
	return (id > 0) ? id : -1;
}

// Receive client successfully got chunk queue : CLT_2_TKR_CLIENT_GOT

// Receive notice that client failed to get chunk queue : CLT_2_TKR_CLIENT_GET_FAILED

// Receive client request for master JFS : CLT_2_TKR_CLIENT_REQ_MASTER
// 	tnt : (not claimed) thread block
// 	ret : (static) client id if there's an outstanding request, else -1
int receive_master_request(TNT * tnt) {
	_TNT_t * thread_block = (_TNT_t *)tnt;
	int id = (int)(long)asyncqueue_pop(thread_block->queues_to_tracker[CLT_2_TKR_CLIENT_REQ_MASTER]);
	return (id > 0) ? id : -1;
}

/* ----- sending ----- */

// Sends a serialized filesystem of diffs to client : TKR_2_CLT_TXN_UPDATE
// 	tnt : (not claimed) thread block
// 	additions : (claimed) additions JFS
// 	deletions : (claimed) deletions JFS
// 	clientid : (static) which client to send to
// 	ret : (static) 1 is success, -1 is failure ()
int send_transaction_update(TNT * tnt, FileSystem * additions, FileSystem * deletions, int clientid) {
	if (!tnt || !additions || !deletions)
		return -1;

	return -1;
}

// Send file acquisition update (client got # chunk of @ file) : TKR_2_CLT_FILE_ACQ

// Sends file system update (client updated @ file) : TKR_2_CLT_FS_UPDATE
// 	tnt : (not claimed) thread block
// 	destination_client : (static) which client to send to
//	originator_client : (static) which client originated the FS update
// 	additions : (not claimed) additions part of FS to update -- tracker logic must claim when all done
// 	deletions : (not claimed) deletions part of FS to update -- tracker logic must claim when all done
int send_FS_update(TNT * thread_block, int destination_client, int originator_client, FileSystem * additions, FileSystem * deletions) {
	if (!thread_block || !additions || !deletions) {
		fprintf(stderr,"send_FS_update error: null tnt, additions FS, deletions FS\n");
		return -1;
	}

	_TNT_t * tnt = (_TNT_t *)thread_block;
	if (!get_peer_by_id(tnt->peer_table, destination_client)) {
		fprintf(stderr,"send_FS_update error: cannot find client %d\n", destination_client);
		return -1;
	}

	// create queue item
	tracker_data_t * queue_item = (tracker_data_t *)malloc(sizeof(tracker_data_t));
	queue_item->client_id = destination_client;

	int length1 = -1, length2 = -1;
	char * buf1, * buf2;
	filesystem_serialize(additions, &buf1, &length1);
	filesystem_serialize(deletions, &buf2, &length2);

	// make one long buffer for both additions and deletions
	queue_item->data = (void *)malloc(length1 + length2 + sizeof(int));
	queue_item->data_len = length1 + length2 + sizeof(int);

	// move serialized filesystems into adjacent buffer and add source id
	memcpy(queue_item->data, buf1, length1);
	memcpy( (char *)((long)queue_item->data + (long)length1), buf2, length2);
	memcpy( (char *)((long)queue_item->data + (long)length1 + (long)length2), &originator_client, sizeof(int));

	free(buf1);
	free(buf2);

	asyncqueue_push(tnt->queues_from_tracker[TKR_2_CLT_FS_UPDATE], queue_item);

	return length1 + length2;
}

// send to all peers to notify that a new peer has appeared : TKR_2_CLT_ADD_PEER
// 	thread_block : (not claimed)
//	client_id : id of client to send out to all peers (SEND_ALL_PEERS) -> send whole table
// 	returns 1 on success and -1 on failure
int send_peer_added(TNT * thread, int destination_client_id, int new_client_id) {
	if (!thread)
		return -1;

	_TNT_t * tnt = (_TNT_t *)thread;

	tracker_data_t * queue_item = (tracker_data_t *)malloc(sizeof(tracker_data_t));
	queue_item->client_id = destination_client_id;
	queue_item->data_len = 0;
	queue_item->data = (void *)(long)new_client_id;

	asyncqueue_push(tnt->queues_from_tracker[TKR_2_CLT_ADD_PEER], (void *)queue_item);
	return 1;
}

// send to all peers to notify that peer has disappeared : TKR_2_CLT_REMOVE_PEER
int send_peer_removed(TNT * thread_block, int destination_client_id, int removed_client_id) {
	_TNT_t * tnt = (_TNT_t *)thread_block;

	tracker_data_t * queue_item = (tracker_data_t* )malloc(sizeof(tracker_data_t));
	queue_item->client_id = destination_client_id;
	queue_item->data_len = 0;
	queue_item->data = (void *)(long)removed_client_id;

	asyncqueue_push(tnt->queues_from_tracker[TKR_2_CLT_REMOVE_PEER], (void *)queue_item);
	return 1;
}

// send the master file system to client : TKR_2_CLT_SEND_MASTER
// 	returns 1 if successful else -1
int send_master(TNT * tnt, int client_id, FileSystem * fs) {
	_TNT_t * thread_block = (_TNT_t *)tnt;
	if (!thread_block || !fs) {
		fprintf(stderr,"send master error: thread block or fs is null\n");
		return -1;
	}

	tracker_data_t * queue_item = (tracker_data_t *)malloc(sizeof(tracker_data_t));
	queue_item->client_id = client_id;
	filesystem_serialize(fs, (char **)&queue_item->data, &queue_item->data_len);
	
	asyncqueue_push(thread_block->queues_from_tracker[TKR_2_CLT_SEND_MASTER], (void *)queue_item);
	return 1;
}

// send master FileTable to client
//	tnt : (not claimed) thread block
// 	client_id : (static) client to send to
// 	ft : (not claimed) FileTable to send
// 	ret : (static) 1 on success, -1 on failure
int send_master_filetable(TNT * thread_block, int client_id, FileTable * ft) {
	if (!thread_block || !ft) {
		fprintf(stderr,"send_master_filetable error: null arguments\n");
		return -1;
	}

	_TNT_t * tnt = (_TNT_t *)thread_block;

	if (!get_peer_by_id(tnt->peer_table, client_id)) {
		fprintf(stderr, "send_master_filetable error: cannot find client %d\n", client_id);
		return -1;
	}

	tracker_data_t * queue_item = (tracker_data_t *)malloc(sizeof(tracker_data_t));
	queue_item->client_id = client_id;
	filetable_serialize(ft, (char **)&queue_item->data, &queue_item->data_len);
	if (queue_item->data_len < 1) {
		fprintf(stderr, "send_master_filetable error: did not serialize any bytes\n");
		return -1;
	}	

	asyncqueue_push(tnt->queues_from_tracker[TKR_2_CLT_SEND_MASTER_FT], (void*)queue_item);
	return 1;
}

/* ###################### *
 * 
 * Things the network can do to interact with the tracker
 *
 * ###################### */

// notify logic about client status update : 
//	tnt : (not claimed) thead block
// 	data : (not claimed)
//	returns 1 on success and -1 on failure
int notify_client_status(_TNT_t * tnt, client_data_t * data) {
	if (!tnt || !data) 
		return -1;

	asyncqueue_push(tnt->queues_to_tracker[CLT_2_TKR_CUR_STATE],(void *)data);
	return 1;
}

// notify logic that client has updated a file
// 	tnt : (not claimed) thread block
// 	queue_item : (not claimed) queue item to push
int notify_client_update(_TNT_t * tnt, client_data_t * queue_item) {
	if (!tnt || !queue_item)
		return -1;

	asyncqueue_push(tnt->queues_to_tracker[CLT_2_TKR_FILE_UPDATE], (void *)queue_item);
	return 1;
}

// notify logic about new client
// 	tnt : (not claimed) thread block
// 	new_client : (not claimed) don't claim this!
// 	returns 1 on success and -1 on failure
int notify_new_client(_TNT_t * tnt, peer_t * new_client) {
	if (!tnt || !new_client)
		return -1;

	asyncqueue_push(tnt->queues_to_tracker[CLT_2_TKR_NEW_CLIENT],(void *)(long)new_client->id);
	return 1;
}

// notify about client lost
int notify_client_lost(_TNT_t * tnt, peer_t * lost_client) {
	if (!tnt || !lost_client) {
		return -1;
	}

	asyncqueue_push(tnt->queues_to_tracker[CLT_2_TKR_REMOVE_CLIENT],(void *)(long)lost_client->id);
	return 1;
}

int notify_client_lost_by_id(_TNT_t * tnt, int lost_client_id) {
	if (!tnt)
		return -1;

	asyncqueue_push(tnt->queues_to_tracker[CLT_2_TKR_REMOVE_CLIENT],(void *)(long)lost_client_id);
	return 1;
}
// notify about client file acquiring update
int notify_client_got(_TNT_t * tnt, client_data_t * queue_item){
    if (!tnt || !queue_item)
        return -1;

    asyncqueue_push(tnt->queues_to_tracker[CLT_2_TKR_CLIENT_GOT], (void *)queue_item);
    return 1;
}

// notify about file acquiring update (success and failure)

// notify that client has requested master : CLT_2_TKR_CLIENT_REQ_MASTER
int notify_master_req(_TNT_t * tnt, int client_id) {
	if (!tnt || client_id < 1)
		return -1;

	asyncqueue_push(tnt->queues_to_tracker[CLT_2_TKR_CLIENT_REQ_MASTER],(void *)(long)client_id);
	return 1;
}

/* ###################### *
 * 
 * The Tracker Network thread functions are below
 *
 * ###################### */

/* ------------------------ FUNCTION DECLARATIONS ------------------------ */

/* --- Network Side Packet Functions --- */

// returns listening socket fd
int open_listening_port(); // HERE

// spins up second thread to accept new client connections -- unused
//void * accept_connections(void * listening_socket);

int send_client_message(_TNT_t * tnt, tracker_data_t * data, tracker_to_client_t type);
int handle_client_msg(int sockfd, _TNT_t * tnt);
void check_liveliness(_TNT_t * tnt); 	// examines if heartbeat has been sent

void send_peer_table_to_client(_TNT_t * tnt, int new_client); 	// sends entire serialized peer table to client

/* --- Network Side Queue Functions --- */

void poll_queues(_TNT_t * tnt);
void check_txn_update_q(_TNT_t * tnt);
void check_file_acq_q(_TNT_t * tnt);
void check_fs_update_q(_TNT_t * tnt);
void check_add_peer_q(_TNT_t * tnt);
void check_remove_peer_q(_TNT_t * tnt);
void check_send_master_q(_TNT_t * tnt);


/* ------------------------ TRACKER NETWORK THREAD ------------------------ */

void * tkr_network_start(void * arg) {

	_TNT_t * tnt = (_TNT_t*)arg;

	// for receiving new connections
	struct sockaddr_in clientaddr;
	unsigned int addrlen = sizeof(clientaddr);
	peer_t * new_client;
	int new_sockfd;

	// select fd sets
	fd_set read_fd_set, active_fd_set;
	FD_ZERO(&active_fd_set);

	// set up timer
	struct timeval timeout;
	timeout.tv_sec = 3;
	timeout.tv_usec = 0;

	// open connection on listening port
	tnt->listening_sockfd = open_listening_port();
	if (tnt->listening_sockfd < 0) {
		fprintf(stderr, "tracker network thread failed to open listening port\n");
		exit(1);
	}

	// listen on it
	if (listen(tnt->listening_sockfd, INIT_CLIENT_NUM) != 0) {
		perror("network tracker failed to listen on socket");
		return (void *)1;
	}

	// add listening socket to fd set
	FD_SET(tnt->listening_sockfd, &active_fd_set);

	int connected = 1;
	while (connected) {

		read_fd_set = active_fd_set;
		if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, &timeout) < 0) {
			fprintf(stderr, "network tracker failed to select amongst inputs\n");
			connected = 0;
			continue;
		}

		//printf("network polling connections...\n");
		for (int i = 0; i < FD_SETSIZE; i++) {
			if (FD_ISSET(i, &read_fd_set)) {

				/* client opening new connection */
				if (i == tnt->listening_sockfd) {
					printf("\nnetwork tracker received new client connection\n");
					new_sockfd = accept(tnt->listening_sockfd, (struct sockaddr *)&clientaddr, &addrlen);
					if (new_sockfd < 0) {
						perror("network tracker failed to accept new connection");
						connected = 0;
						continue;
					}

					// add new peer to table
					if ((new_client = add_peer(tnt->peer_table, (char *)&clientaddr.sin_addr, new_sockfd)) == NULL) {
						fprintf(stderr,"network tracker received peer connection but couldn't add it to the table\n");
						continue;
					}

					FD_SET(new_sockfd, &active_fd_set);

					// notify tracker
					notify_new_client(tnt, new_client);
					send_peer_table_to_client(tnt, new_client->socketfd);

					// DEBUG -- 
					printf("Updated Peer Table\n");
					print_table(tnt->peer_table);

					printf("NETWORK -- added new client %d on socket %d\n", new_client->id, new_client->socketfd);

				/* data on existing connection */
				} else {

					printf("\nnetwork tracker received message from client on socket %d\n", i);
					if (handle_client_msg(i, tnt) != 1) {
						fprintf(stderr,"failed to handle client message on socket %d\n", i);

						peer_t * lost_client = get_peer_by_socket(tnt->peer_table, i);
						if (lost_client != NULL) {

							// tracker should send out the updated file system to clients
							notify_client_lost_by_id(tnt, lost_client->id);
							delete_peer(tnt->peer_table, lost_client->id);
						}
						
						// DEBUG -- 
						printf("Updated Peer table\n");
						print_table(tnt->peer_table);

						// don't listen to broken connection
						FD_CLR(i, &active_fd_set);
						close(i);
					}

				}
			}
		} 	// end of socket checks
        
		/* check time last alive */
		check_liveliness(tnt);

    /* check queues and do important stuff */
		poll_queues(tnt);		
	}

	close(tnt->listening_sockfd);

	printf("network tracker ending\n");
	return (void *)1;
}

/* ------------------------ HANDLE PACKETS ON NETWORK ------------------------ */

// http://beej.us/guide/bgnet/output/html/multipage/syscalls.html#getaddrinfo
int open_listening_port() {

	// IPv4 #####################################
	struct sockaddr_in serv_addr;
	int listening_sockfd;
	
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	//serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(TRACKER_LISTENING_PORT);

	listening_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	assert(listening_sockfd>0);

	int opt_yes = 1;
	if (setsockopt(listening_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt_yes, sizeof(int)) == -1) {
    perror("Error setsockopt failure");
    return -1;
  } 

	assert(bind(listening_sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == 0);
	// assert(listen(listening_sockfd, incoming_neighbors) == 0);

	printf("NETWORK -- listening at IP address %s on port %d\n", inet_ntoa(serv_addr.sin_addr), TRACKER_LISTENING_PORT);
	return listening_sockfd;
}

// returns 1 if successfully sent, else -1
int send_client_message(_TNT_t * tnt, tracker_data_t * data, tracker_to_client_t type) {

	if (!tnt || !data)
		return -1;

	tracker_pkt_t pkt; 
	peer_t * client = get_peer_by_id(tnt->peer_table, data->client_id);
	if (!client)
		return -1;

	if (client->socketfd < 0)
		return -1;

	pkt.type = type;
	pkt.data_len = data->data_len;

	send(client->socketfd, &pkt, sizeof(pkt), 0);
	send(client->socketfd, data->data, data->data_len, 0);

	return 1;
}

// processes existing client message
// 	if successful, returns 1
int handle_client_msg(int sockfd, _TNT_t * tnt) {
	if (!tnt)
		return -1;

	client_pkt_t pkt;
	char * buf;
	peer_t * client = get_peer_by_socket(tnt->peer_table, sockfd);
	if (!client) {
		fprintf(stderr,"failed to find client on socket %d\n", sockfd);
		return -1;
	}

	// get type and data length
	if (recv(sockfd, &pkt, sizeof(pkt), 0) != sizeof(pkt)) {
		fprintf(stderr,"error receiving header data from client %d\n", client->id);
		return -1;
	}

	// set up data buffer and receive data segment if necessary
	if (pkt.data_len > 0) {
		
		buf = (char *)calloc(1,pkt.data_len);
		if (!buf) {
			fprintf(stderr, "error receiving data -- could not allocate buffer\n");
			return -1;
		}
		
		printf("client %d on socketd %d -- waiting for data\n", client->id, client->socketfd); 	// debug
		fflush(stdout);

		// receive data
		if (recv(sockfd, buf, pkt.data_len, 0) < pkt.data_len) {
			fprintf(stderr,"error receiving data from client %d\n", client->id);
			return -1;
		}
	} else {
		buf = NULL;
	}

	// set up data to pass to logic (how to set this up so it's only created if necessary?)
	client_data_t * client_data = malloc(sizeof(client_data_t));
	client_data->client_id = client->id;

	int rc = -1;
	switch(pkt.type) {
		case HEARTBEAT:
			printf("NETWORK -- received heartbeat from client %d\n", client->id);
			// update time last heard from client
			client->time_last_alive = time(NULL);
			free(client_data);

			rc = 1;
			break;

		case CLIENT_STATE:
			printf("NETWORK -- received state update from client %d\n", client->id);
			FileSystem * fs = filesystem_deserialize(buf, &client_data->data_len);
			if (!fs) {
				printf("error deserializing client state update\n");
				break;
			}
			client_data->data = (void *)fs;
			notify_client_status(tnt, client_data);

			rc = 1;
			break;

		case CLIENT_UPDATE:
			printf("NETWORK -- received client update from client %d\n", client->id);
			client_data->client_id = client->id;
			client_data->data_len = pkt.data_len;
			client_data->data = buf;

			notify_client_update(tnt, client_data);
			buf = NULL;

			rc = 1;
			break;

		case REQUEST_MASTER:
			printf("NETWORK -- received request for master JFS from client %d\n", client->id);
			notify_master_req(tnt, client->id);
			free(client_data);

			rc = 1;
			break;

		default:
			printf("NETWORK -- Unknown packet type (%d) from client id %d\n", pkt.type, client->id);
			break;

	}

	if (buf != NULL) {
		free(buf);
	}

	return rc;
}

void check_liveliness(_TNT_t * tnt) {
	if (!tnt)
		return;

	peer_table_t * pt = tnt->peer_table;

	time_t current_time = time(NULL);
	for (int i = 0; i < pt->size; i++) {
		if (pt->peer_list[i] != NULL) {
			if (pt->peer_list[i]->time_last_alive - current_time > DIASTOLE) {
				printf("NETWORK -- notifying logic that client %d has died\n", pt->peer_list[i]->id);
				notify_client_lost(tnt, pt->peer_list[i]);
				delete_peer(pt, pt->peer_list[i]->id);
			}
		}
	}

}

void send_peer_table_to_client(_TNT_t * tnt, int new_client_fd) {
	if (!tnt || new_client_fd < 0)
		return;

	tracker_pkt_t pkt;
	pkt.type = PEER_TABLE;

	char * buf = serialize_peer_table(tnt->peer_table, &pkt.data_len);
	if (!buf || pkt.data_len < 0) {
		fprintf(stderr, "tracker failed to send peer table to client due to serialization error\n");
		return;
	}

	send(new_client_fd, &pkt, sizeof(pkt), 0);
	send(new_client_fd, buf, pkt.data_len, 0);

	free(buf);
	return;
}

/* ------------------------ HANDLE QUEUES TO NETWORK FROM TRACKER ------------------------ */

void poll_queues(_TNT_t * tnt) {
	check_txn_update_q(tnt);
	check_file_acq_q(tnt);
	check_fs_update_q(tnt);
	check_add_peer_q(tnt);
	check_remove_peer_q(tnt);
	check_send_master_q(tnt);
}

void check_txn_update_q(_TNT_t * tnt) {
	AsyncQueue * q = tnt->queues_from_tracker[TKR_2_CLT_TXN_UPDATE];
	tracker_data_t * queue_item ;
	while ( (queue_item = asyncqueue_pop(q)) != NULL) {

		printf("NETWORK -- sending transaction update -- UNFILLED FUNCTION!\n");
		//free(queue_item->data);
		free(queue_item);
	}
	return;
}

void check_file_acq_q(_TNT_t * tnt) {
	AsyncQueue * q = tnt->queues_from_tracker[TKR_2_CLT_FILE_ACQ];
	tracker_data_t * queue_item ;
	while ( (queue_item = asyncqueue_pop(q)) != NULL) {

		printf("NETWORK -- sending file acquisition update -- UNFILLED FUNCTION!\n");
		//free(queue_item->data);
		free(queue_item);
	}
	return;
}

void check_fs_update_q(_TNT_t * tnt) {
	AsyncQueue * q = tnt->queues_from_tracker[TKR_2_CLT_FS_UPDATE];
	tracker_data_t * queue_item ;
	while ( (queue_item = asyncqueue_pop(q)) != NULL) {

		printf("NETWORK -- sending file system update!\n");

		peer_t * client = get_peer_by_id(tnt->peer_table, queue_item->client_id);
		if (!client) {
			fprintf(stderr,"check_fs_update_q failed to get client %d\n", queue_item->client_id);
			return;
		}

		if (send_client_message(tnt, queue_item, FS_UPDATE) != 1) {
			fprintf(stderr,"failed to send file system update to client %d\n", queue_item->client_id);
		}

		free(queue_item->data);
		free(queue_item);
	}
	return;
}

void check_add_peer_q(_TNT_t * tnt) {
	AsyncQueue * q = tnt->queues_from_tracker[TKR_2_CLT_ADD_PEER];
	tracker_data_t * queue_item ;
	while ( (queue_item = asyncqueue_pop(q)) != NULL) {

		printf("NETWORK -- sending add peer (new peer %d) to client %d\n", queue_item->client_id, (int)queue_item->data);
		peer_t * client = get_peer_by_id(tnt->peer_table, queue_item->client_id);
		if (!client) {
			fprintf(stderr, "check_add_peer_q: failed to find destination client\n");
			return;
		}

		send_peer_table_to_client(tnt, client->socketfd);
		free(queue_item);
	}
	return;
}
void check_remove_peer_q(_TNT_t * tnt) {
	AsyncQueue * q = tnt->queues_from_tracker[TKR_2_CLT_REMOVE_PEER];
	tracker_data_t * queue_item ;
	while ( (queue_item = asyncqueue_pop(q)) != NULL) {

		printf("NETWORK -- sending remove peer (delete peer %d) to client %d\n", queue_item->client_id, (int)queue_item->data);
		peer_t * client = get_peer_by_id(tnt->peer_table, queue_item->client_id);
		if (!client) {
			fprintf(stderr, "check_add_peer_q: failed to find destination client\n");
			return;
		}

		send_peer_table_to_client(tnt, client->socketfd);
		free(queue_item);
	}
	return;
}

// deseminate master to client
void check_send_master_q(_TNT_t * tnt) {
	AsyncQueue * q = tnt->queues_from_tracker[TKR_2_CLT_SEND_MASTER];
	tracker_data_t * queue_item ;
	while ( (queue_item = asyncqueue_pop(q)) != NULL) {

		peer_t * client = get_peer_by_id(tnt->peer_table, queue_item->client_id);

		printf("NETWORK -- sending master JFS to client %d\n", client->id);
		if (send_client_message(tnt, queue_item, MASTER_STATUS) != 1) {
			fprintf(stderr,"failed to send master status update to client %d\n", queue_item->client_id);
		}
		free(queue_item);
	}

	return;
}

void check_send_master_ft_q(_TNT_t * tnt) {
	AsyncQueue * q = tnt->queues_from_tracker[TKR_2_CLT_SEND_MASTER_FT];
	tracker_data_t * queue_item;
	while ((queue_item = asyncqueue_pop(q)) != NULL) {

		printf("NETWORK -- sending master file table to client %d\n", queue_item->client_id);
		if (send_client_message(tnt, queue_item, MASTER_FT) != 1) {
			fprintf(stderr,"failed to send master file table to client %d\n", queue_item->client_id);
		}

		free(queue_item->data);
		free(queue_item);
	}
}

