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

#define INIT_CLIENT_NUM 10

typedef struct _TNT {

	/* 
	 * *** INCOMING QUEUES ****
	 *
	 * -- client to tracker --
	 * queues_to_tracker[0] -> a new client is trying to join, contains current JFS state (to logic)
 	 * queues_to_tracker[1] -> client has just updated a file (to logic)
 	 * queues_to_tracker[2] -> client disconnected queue (to logic)
 	 * queues_to_tracker[3] -> client successful get queue (to logic)
 	 * queues_to_tracker[4] -> client failed to get queue (to logic)
 	 *
 	 */
 	AsyncQueue ** queues_to_tracker; 		// to logic

 	/*
 	 * *** OUGOING QUEUES ***
 	 *
 	 * -- tracker to client --
 	 * queues_from_tracker[0] -> transaction update queue
 	 * queues_from_tracker[1] -> file acquisition status update queue
 	 * queues_from_tracker[2] -> peer added messages to distribute
 	 * queues_from_tracker[3] -> peer removed messages to distribute
 	 *
 	 */
 	AsyncQueue ** queues_from_tracker;  	// from logic

 	/*
 	 * networking monitoring information
 	 */
 	pthread_t thread_id;
 	peer_table_t * peer_table;

} _TNT_t;

// launches network thread
void * tkr_network_start(void * arg);

TNT * StartTrackerNetwork() {

	_TNT_t * tracker_thread = (_TNT_t *)calloc(1,sizeof(_TNT_t));

	/* -- incoming from client to tracker -- */
	tracker_thread->queues_to_tracker = (AsyncQueue **)calloc(5,sizeof(AsyncQueue *));
	tracker_thread->queues_to_tracker[0] = asyncqueue_new();
	tracker_thread->queues_to_tracker[1] = asyncqueue_new();
	tracker_thread->queues_to_tracker[2] = asyncqueue_new();
	tracker_thread->queues_to_tracker[3] = asyncqueue_new();
	tracker_thread->queues_to_tracker[4] = asyncqueue_new();

	/* -- outgoing from tracker to client -- */
	tracker_thread->queues_from_tracker = (AsyncQueue **)calloc(4,sizeof(AsyncQueue *));
	tracker_thread->queues_from_tracker[0] = asyncqueue_new();
	tracker_thread->queues_from_tracker[1] = asyncqueue_new();
	tracker_thread->queues_from_tracker[2] = asyncqueue_new();
	tracker_thread->queues_from_tracker[3] = asyncqueue_new();

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
 * Things that the client can do to interact with network
 *
 * ###################### */

/* ----- receiving ----- */

// Receives a "current state" fs message from client 
//	fs : (not claimed) pointer that will reference deserialized client JFS
// 	clientid : (not claimed) pointer to int that will be filled with client id
// 	ret : (static) 1 is success, -1 is failure (communications broke) 
int receive_client_state(TNT * tnt, FileSystem ** fs, int * clientid);

// client file system update (got and failed to get)
// 	tnt : (not claimed) thread block 
//	clientid : (not claimed) space for client id that will be filled if update is there
// 	ret : (not claimed) client's update JFS (new minus original)
FileSystem * receive_client_update(TNT * tnt, int * clientid);

// client added

// client deleted

/* ----- sending ----- */

// Sends a serialized filesystem of diffs to client
// 	fs : (not claimed) pointer to diff FileSystem
// 	clientid : (static) which client to send to
// 	ret : (static) 1 is success, -1 is failure ()
int send_transaction_update(TNT * tnt, FileSystem * fs, int clientid);

// Sends file system update
int send_FS_update();

// send to all peers to notify that a new peer has appeared
int send_peer_added(TNT * tnt);

// send to all peers to notify that peer has disappeared
int send_peer_removed(TNT * tnt);

/* ###################### *
 * 
 * The Tracker Network thread functions are below
 *
 * ###################### */

// returns listening socket fd
int open_listening_port();

// spins up a secondary thread to accept new peer connections
void * accept_connections(void * listening_socket);

void * tkr_network_start(void * arg) {

	int listening_sockfd;

	// init peer table

	// open connection on listening port


	// accept incoming connections -> put new client request onto queues_to_tracker[0]; add to peer table

	// poll open 

		// wait for incoming peer connections 
		// handle inquisitive peers when they connect 
		// -> put message on queue to app logic that client needs to initiate handshake

	// continue to poll open thread connections
		// "heartbeat messages"
		// receive transaction update messages from peers

	return (void *)1;
}

void * accept_connections(void * listening_socket_arg) {
	int listenfd = *(int *)listening_socket_arg;

	return (void *)1;
}

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
		exit(1);
	}

	int listening_sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
	if (listening_sockfd < 0) {
		fprintf(stderr, "tracker - open_listening_port: socket error\n");
		exit(1);
	}

	if (bind(listening_sockfd, servinfo->ai_addr, servinfo->ai_addrlen) < 0) {
		perror("tracker - open_listening_port bind error");
		exit(1);
	}

	freeaddrinfo(servinfo); 	// free filled out structure

	return listening_sockfd;
}
