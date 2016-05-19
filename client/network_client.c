/*
 * network_client.c
 *
 * contains functionality for client-side network thread
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

#include "network_client.h"
#include "../common/constant.h"
#include "../common/peer_table.h"
#include "../utility/AsyncQueue/AsyncQueue.h" 
#include "../utility/FileSystem/FileSystem.h" 

#define INIT_PEER_SIZE 10

typedef struct _CNT {

	/*
	 * *** INCOMING QUEUES ***
	 * 
	 * -- tracker 2 client --
	 * tkr_queues_to_client[0] -> transaction update queue (JFS of updated files)
	 * tkr_queues_to_client[1] -> file acquisition status queue
	 * tkr_queues_to_client[2] -> peer added queue
	 * tkr_queues_to_client[3] -> peer deleted queue
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
	 * tkr_queue_from_client[0] -> send current status / (join network)
	 * tkr_queue_from_client[1] -> send acquisition update to tracker (successful & unsucessful)
	 * tkr_queue_from_client[2] -> send quit message
	 *
	 * -- client 2 client --
	 * clt_queue_from_client[0] -> send request for chunk
	 * clt_queue_from_client[1] -> send chunk
	 * clt_queue_from_client[2] -> send an error to client
	 *
	 */
	AsyncQueue ** tkr_queue_from_client; 	// from logic to tracker
	AsyncQueue ** clt_queue_from_client; 	// from logic to client

	/*
	 * network monitoring information
	 */
	pthread_t thread_id;
	char * ip_addr;
	int ip_len;
	peer_table_t * peer_table;

} _CNT_t;

// network thread start point
void * clt_network_start(void * arg);

CNT * StartClientNetwork(char * ip_addr, int ip_len) {

	if (!ip_addr) {
		fprintf(stderr,"StartClientNetwork: bad ip address\n");
		return NULL;
	}

	_CNT_t * client_thread = (_CNT_t *)calloc(1,sizeof(_CNT_t));

	/* -- incoming client to tracker -- */
	client_thread->tkr_queues_to_client = (AsyncQueue **)calloc(4, sizeof(AsyncQueue *));
	client_thread->tkr_queues_to_client[0] = asyncqueue_new();
	client_thread->tkr_queues_to_client[1] = asyncqueue_new();
	client_thread->tkr_queues_to_client[2] = asyncqueue_new();

	/* -- incoming client to client -- */
	client_thread->clt_queues_to_client = (AsyncQueue **)calloc(2,sizeof(AsyncQueue *));
	client_thread->clt_queues_to_client[0] = asyncqueue_new();
	client_thread->clt_queues_to_client[1] = asyncqueue_new();

	/* -- outgoing client to tracker -- */
	client_thread->tkr_queue_from_client = (AsyncQueue **)calloc(3,sizeof(AsyncQueue *));
	client_thread->tkr_queue_from_client[0] = asyncqueue_new();
	client_thread->tkr_queue_from_client[1] = asyncqueue_new();
	client_thread->tkr_queue_from_client[2] = asyncqueue_new();

	/* -- outgoing client to client -- */
	client_thread->clt_queue_from_client = (AsyncQueue **)calloc(3,sizeof(AsyncQueue *));
	client_thread->clt_queue_from_client[0] = asyncqueue_new();
	client_thread->clt_queue_from_client[1] = asyncqueue_new();
	client_thread->clt_queue_from_client[2] = asyncqueue_new();

	/* -- save tracker server arguments -- */
	client_thread->ip_addr = malloc(ip_len);
	memcpy(client_thread->ip_addr, ip_addr, ip_len);
	client_thread->ip_len = ip_len;

	client_thread->peer_table = init_peer_table(INIT_PEER_SIZE);
	if (!client_thread->peer_table) {
		fprintf(stderr,"client network thread failed to create peer table\n");
		return NULL;
	}

	/* -- spin off network thread -- */
	pthread_create(&client_thread->thread_id, NULL, clt_network_start, client_thread);

	return (CNT *)client_thread;
}

// nicely ends network
void EndNetwork() {
	printf("Client Network Thread ending network (badly right now)\n");
	exit(1);
}

/* ###################### *
 * 
 * Things that the client should do to interact with network thread
 *
 * ###################### */

/* ----- receiving ----- */

// receive transaction update ->
FileSystem * recv_diff(CNT * thread) {

	return NULL;
}

// receive acq status update

// get peer added message

// get peer deleted message

// receive request for chunk

// receive chunk from peer

/* ----- sending ----- */
int send_status(CNT * thread, FileSystem * fs) {

	// serialize file system

	// forward to tracker

	return -1;
}

/* 
 * this queue must do the following things
 * 	- send a current status update (JFS)
 * 	- send a request for a file chunk
 * 	- send a file chunk
 * 	- send an error (associated with an attempt to get a chunk)
 * 	- send an update to tracker about acquired chunk
 * 	- send a "I'm quitting message"
 */




/* ###################### *
 * 
 * Network Thread functions
 *
 * ###################### */

/* ------------------------ FUNCTION DECLARATIONS ------------------------ */

// returns listening socket fd
int connect_to_tracker();

/* ------------------------ NETWORK THREAD ------------------------ */

void * clt_network_start(void * arg) {

	_CNT_t * cnt = (_CNT_t *)arg;

	int tracker_fd;
	while ( (tracker_fd = connect_to_tracker()) < 0) {
		printf("Failed to connect with tracker...\n");
		sleep(5);
	}

	// set up timer
	struct timeval timeout;
	timeout.tv_sec = 3;
	timeout.tv_usec = 0;

	// select fd sets
	fd_set read_fd_set, active_fd_set;
	FD_ZERO(&active_fd_set);

	// add listening socket to fd set
	FD_SET(tracker_fd, &active_fd_set);

	/* need to open peer listening socket */
	int peer_listening_fd = -1;

	int connected = 1;
	while (connected) {

		read_fd_set = active_fd_set;
		if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, &timeout) < 0) {
			fprintf(stderr, "network client failed to select amongst inputs\n");
			connected = 0;
			continue;
		}

		printf("client network polling connections...\n");
		for (int i = 0; i < FD_SETSIZE; i++) {
			if (FD_ISSET(i, &read_fd_set)) {

				// new connection
				if (i == tracker_fd) {

					printf("Receiving message from tracker\n");
					handle_tracker_msg(tracker_fd, cnt);
					

				// existing connection
				} else if (i == peer_listening_fd) {

					printf("Receiving new connection from peer\n");
					handle_peer_msg(i, cnt);

					// new_sockfd = accept(new_sockfd, (struct sockaddr *)&clientaddr, &addrlen);
					// if (new_sockfd < 0) {
					// 	fprintf(stderr,"network tracker accept_connections thread failed to accept new connection\n");
					// 	continue;
					// }

				}
			}

		}

	}

	return (void *)1;
}

/* ------------------------ FUNCTION DEFINITIONS ------------------------ */

// connects to the tracker
//	returns connected socket if successful
// 	else returns -1
int connect_to_tracker(int ip_len, char * ip_addr) {
	if (!ip_addr)
		return -1;

	int sockfd;
	if ((sockfd = socket(AF_INET6, SOCK_STREAM, 6)) < 0) {
		perror("client_network thread failed to create tracker socket");
		return -1;
	}

	// fill out server struct
	struct sockaddr_in6 servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	memcpy(&servaddr.sin6_addr, ip_addr, ip_len);
	servaddr.sin6_family = AF_INET6;
	servaddr.sin6_port = htons(TRACKER_LISTENING_PORT); 

	// connect
	if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
		perror("client_network thread failed to connect to tracker");
		return -1;
	}

	return sockfd;
}

int handle_tracker_msg(int tracker_fd, _CNT_t * cnt) {
	if (!cnt)
		return -1;

	tracker_pkt_t pkt;
	if (recv(tracker_fd, &pkt, sizeof(tracker_pkt_t)) != sizeof(tracker_pkt_t)){
		fprintf(stderr,"client network failed to get message from tracker\n");
		return -1;
	}

	char * buf = malloc(pkt.data_len);

	switch (pkt.type) {
		case TRANSACTION_UPDATE:
			if (recv(tracker_fd, buf, pkt.data_len) != pkt.data_len) {
				fprintf(stderr, "client network has error receiving data in transaction update\n");
				break;
			}
			

			break;

		case FILE_ACQ_UPDATE:
		case PEER_ADDED:
		case PEER_DELETED:
		default:
			printf("CLIENT NETWORK received unhandled packet of type %d\n", pkt.type);
			break;
	}


}
