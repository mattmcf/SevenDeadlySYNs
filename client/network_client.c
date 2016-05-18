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


typedef struct _clt_network_thread {
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
} _clt_network_thread_t;

typedef struct clt_network_arg {

	/* following fields designate which IP address to connect to */
	int tracker_ip_len;
	char tracker_ip[IP_MAX_LEN]; 

	//AsyncQueue ** tkr_queues_from_client; 	// from logic to tracker
	//AsyncQueue ** clt_queues_from_client; 	// from logic to clients
}

// pass me a pointer to clt_network_arg struct with filled out queues
void * clt_network_start(void *);

/* ###################### *
 * 
 * Things that the client should do to interact with network thread
 *
 * ###################### */

ClientNetworkThread * StartClientNetwork(char * ip_addr, int ip_len) {
	if (!ip_addr) {
		fprintf(stderr,"StartClientNetwork: bad ip address\n");
		return NULL;
	}



}


// sends current file system to network which will send to tracker
int send_client_state(FileSystem * fs);

// send an acquisition update to tracker letting it know that a file has been acquired
int send_acq_update(/* what does this struct look like */);


// returns listening socket fd
int connect_to_tracker();

void * clt_network_start(void *) {

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
	if ( (sockfd = socket(AF_INET6, SOCK_STREAM, 6)) < 0) {
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