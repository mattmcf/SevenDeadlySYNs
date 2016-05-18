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

#include "network_client.c"
#include "../utility/AsyncQueue.h"

typedef struct _ClientNetworkThread {

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

} _ClientNetworkThread_t;


ClientNetworkThread * StartClientNetwork(char * ip_addr, int ip_len) {

	if (!ip_addr) {
		fprintf(stderr,"StartClientNetwork: bad ip address\n");
		return NULL;
	}

	_ClientNetworkThread_t * client_thread = (_ClientNetworkThread_t *)calloc(1,sizeof(_ClientNetworkThread_t));

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

	/* -- spin off network thread -- */
	pthread_create(client_thread, NULL, clt_network_start, client_thread);

	return (ClientNetworkThread *)client_thread;
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

// sends current file system to network which will send to tracker
int send_client_state(FileSystem * fs);

// send an acquisition update to tracker letting it know that a file has been acquired
int send_acq_update(/* what does this struct look like */);

/* ###################### *
 * 
 * Things that the network thread will do
 *
 * ###################### */

// returns listening socket fd
int connect_to_tracker();

void * clt_network_start(void * arg) {

	_ClientNetworkThread_t * cnt = (_ClientNetworkThread_t *)arg;

	int tracker_fd;
	while ( (tracker_fd = connect_to_tracker) < 0) {
		printf("Failed to connect with tracker...\n");
		sleep(5);
	}



}

// connects to the tracker
//	returns connected socket if successful
// 	else returns -1
int connect_to_tracker(int ip_len, char * ip_addr) {
	if (!ip_addr)
		return -1

	int sockfd;
	if ((sockfd = socket(AF_INET6, SOCK_STREAM, 6)) < 0) {
		perror("client_network thread failed to create tracker socket");
		return -1;
	}

	// fill out server struct
	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	memcpy(&servaddr.sin_addr.s_addr, ip_addr, ip_len);
	servaddr.sin_family = AF_INET6;
	servaddr.sin_port = htons(TRACKER_LISTENING_PORT); 

	// connect
	if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
		perror("client_network thread failed to connect to tracker");
		return -1;
	}

	return sockfd;
}