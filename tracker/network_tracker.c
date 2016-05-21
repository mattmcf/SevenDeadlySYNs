/*
 * network_tracker.c
 *
 * contains functionality for the tracker's network thread
 */

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

#include "network_tracker.h"
#include "../utility/AsyncQueue/AsyncQueue.h"
#include "../common/constant.h"
#include "../common/peer_table.h"
#include "../common/packets.h"

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

typedef struct _TNT {

	/* 
	 * *** INCOMING QUEUES ****
	 *
	 * -- client to tracker --
	 * queues_to_tracker[0] -> contains client's curent JFS (to logic)
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
	tracker_thread->queues_from_tracker = (AsyncQueue **)calloc(6,sizeof(AsyncQueue *));
	tracker_thread->queues_from_tracker[TKR_2_CLT_TXN_UPDATE] = asyncqueue_new();
	tracker_thread->queues_from_tracker[TKR_2_CLT_FILE_ACQ] = asyncqueue_new();
	tracker_thread->queues_from_tracker[TKR_2_CLT_FS_UPDATE] = asyncqueue_new();
	tracker_thread->queues_from_tracker[TKR_2_CLT_ADD_PEER] = asyncqueue_new();
	tracker_thread->queues_from_tracker[TKR_2_CLT_REMOVE_PEER] = asyncqueue_new();
	tracker_thread->queues_from_tracker[TKR_2_CLT_SEND_MASTER] = asyncqueue_new();

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

void EndNetwork() {
	printf("Tracker Network Thread ending network (badly right now)\n");
	exit(1);
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

	if (!data_pkt != null) {

		*fs = data_pkt->data;
		*clientid = data_pkt->client_id;
		free(data_pkt);
		return 1

	} else {
		return -1;
	}
}

// Receive client file system update (modified file) : CLT_2_TKR_FILE_UPDATE
// 	tnt : (not claimed) thread block 
//	clientid : (not claimed) space for client id that will be filled if update is there
// 	ret : (not claimed) client's update JFS (new minus original) (null on failure)
FileSystem * receive_client_update(TNT * tnt, int * clientid) {

	return NULL;
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
int send_FS_update(TNT * tnt) {
	return -1;
}

// send to all peers to notify that a new peer has appeared : TKR_2_CLT_ADD_PEER
int send_peer_added(TNT * tnt) {
	return -1;
}

// send to all peers to notify that peer has disappeared : TKR_2_CLT_REMOVE_PEER
int send_peer_removed(TNT * tnt) {
	return -1;
}

// send the master file system to client : TKR_2_CLT_SEND_MASTER
// 	returns 1 if successful else -1
int send_master(TNT * tnt, int client_id, FileSystem * fs) {
	_TNT_t * thread_block = (_TNT_t *)tnt;
	tracker_data_t * queue_item = (tracker_data_t *)malloc(sizeof(tracker_data_t));
	queue_item->client_id = client_id;

	filesystem_serialize(fs, queue_item->data, &queue_item->data_len);
	
	asyncqueue_push(thread_block->queues_from_tracker[TKR_2_CLT_SEND_MASTER], (void *)queue_item);
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

// notify about file acquiring update (success and failure)


/* ###################### *
 * 
 * The Tracker Network thread functions are below
 *
 * ###################### */

/* ------------------------ FUNCTION DECLARATIONS ------------------------ */

/* --- Network Side Packet Functions --- */

// returns listening socket fd
int open_listening_port();

// spins up second thread to accept new client connections -- unused
//void * accept_connections(void * listening_socket);

int send_client_message(_TNT_t * tnt, tracker_data_t * data, tracker_to_client_t type);
int handle_client_msg(int sockfd, _TNT_t * tnt);
void check_liveliness(_TNT_t * tnt);

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
	struct sockaddr_in6 clientaddr;
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

		printf("network polling connections...\n");
		for (int i = 0; i < FD_SETSIZE; i++) {
			if (FD_ISSET(i, &read_fd_set)) {

				/* client opening new connection */
				if (i == tnt->listening_sockfd) {
					printf("network tracker received new client connection\n");
					new_sockfd = accept(new_sockfd, (struct sockaddr *)&clientaddr, &addrlen);
					if (new_sockfd < 0) {
						fprintf(stderr,"network tracker accept_connections thread failed to accept new connection\n");
						continue;
					}

					FD_SET(new_sockfd, &active_fd_set);

					// add new peer to table
					if ((new_client = add_peer(tnt->peer_table, &clientaddr.sin6_addr, new_sockfd)) == NULL) {
						fprintf(stderr,"network tracker received peer connection but couldn't add it to the table\n");
						continue;
					}

					// notify tracker
					notify_new_client(tnt, new_client);

				/* data on existing connection */
				} else {

					handle_client_msg(i, tnt);

				}
			}
		} 	// end of socket checks
        

		/* check time last alive */
		check_liveliness(tnt);

    /* check queues and do important stuff */
		poll_queues(tnt);		
	}

	printf("network tracker ending\n");
	return (void *)1;
}

/* ------------------------ HANDLE PACKETS ON NETWORK ------------------------ */

// http://beej.us/guide/bgnet/output/html/multipage/syscalls.html#getaddrinfo
int open_listening_port() {

	struct addrinfo hints;
	struct addrinfo *servinfo;

	char port_str[10];
	sprintf(port_str, "%d", TRACKER_LISTENING_PORT);

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET6; 			// use IPv6
	hints.ai_socktype = SOCK_STREAM; 	// tcp
	hints.ai_flags = AI_PASSIVE; 			// fill ip for me

	if (getaddrinfo(NULL,port_str,&hints,&servinfo) != 0) {
		perror("tracker - open_listening_port getaddrinfo error");
		return -1;
	}

	int listening_sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
	if (listening_sockfd < 0) {
		fprintf(stderr, "tracker - open_listening_port: socket error\n");
		return -1;
	}

	if (bind(listening_sockfd, servinfo->ai_addr, servinfo->ai_addrlen) < 0) {
		perror("tracker - open_listening_port bind error");
		return -1;
	}

	freeaddrinfo(servinfo); 	// free filled out structure

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
			client->time_last_alive = time();
			free(client_data);
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

	time_t current_time = time();
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

	return;
}

void check_file_acq_q(_TNT_t * tnt) {
	AsyncQueue * q = tnt->queues_from_tracker[TKR_2_CLT_FILE_ACQ];

	return;
}

void check_fs_update_q(_TNT_t * tnt) {
	AsyncQueue * q = tnt->queues_from_tracker[TKR_2_CLT_FS_UPDATE];

	return;
}

void check_add_peer_q(_TNT_t * tnt) {
	AsyncQueue * q = tnt->queues_from_tracker[TKR_2_CLT_ADD_PEER];

	return;
}
void check_remove_peer_q(_TNT_t * tnt) {
	AsyncQueue * q = tnt->queues_from_tracker[TKR_2_CLT_REMOVE_PEER];

	return;
}

// deseminate master to client
void check_send_master_q(_TNT_t * tnt) {
	AsyncQueue * q = tnt->queues_from_tracker[TKR_2_CLT_SEND_MASTER];
	tracker_data_t * queue_item = (tracker_data_t *)asyncqueue_pop(q);
	if (queue_item != NULL) {
		peer_t * client = get_peer_by_id(tnt->peer_table, queue_item->client_id);

		printf("NETWORK -- sending master JFS to client %d\n", queue_item->client_id);
		if (send_client_message(tnt, queue_item, MASTER_STATUS) != 1) {
			fprintf(stderr,"failed to send master status update to client %d\n", queue_item->client_id);
		}
		free(queue_item);
	}

	return;
}

/* ###################### *
 * 
 * The Tracker New Connections Thread is below -- UNUSED
 *
 * ###################### */

// void * accept_connections(void * listening_socket_arg) {

// 	_TNT_t * thread_block = (_TNT_t*)arg;
// 	struct sockaddr_in6 clientaddr;
// 	int addrlen = sizeof(addr);
// 	peer_t * new_client;

// 	int connected = 1, new_sockfd;

// 	// listen on it
// 	if (listen(thread_block->listening_sockfd, INIT_CLIENT_NUM) != 0) {
// 		perror("network tracker accept_connections thread failed to listen on socket");
// 		return (void *)1;
// 	}

// 	while (connected) {
// 		new_sockfd = accept(new_sockfd, (struct sockaddr *)&clientaddr, &addrlen);
// 		if (new_sockfd < 0) {
// 			fprintf(stderr,"network tracker accept_connections thread failed to accept new connection\n");
// 			connected = 0;
// 			continue;
// 		}

// 		// add new peer to table
// 		if ((new_client = add_peer(thread_block->peer_table, &clientaddr.sin6_addr, sizeof(in6_addr))) == NULL) {
// 			fprintf(stderr,"network tracker accept_connections thread received peer connection but couldn't add it to the table\n");
// 			continue;
// 		}

// 		new_client->socketfd = new_sockfd;
// 		new_client->time_last_last = time();

// 		// notify tracker
// 		notify_new_client(thread_block, new_client);

// 		// TODO queue how to add new socket to list of tracked sockets???
// 	}

// 	printf("network tracker accept_connections thread is ending\n");
// 	close(thread_block->listening_sockfd);
// 	return (void *)1;
// }
