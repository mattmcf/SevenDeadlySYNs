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

// returns listening socket fd
int open_listening_port();

// spins up a secondary thread to accept new peer connections
void * accept_connections(void * listening_socket);

/* ###################### *
 * 
 * Working loop...
 *
 * ###################### */

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
}

void * accept_connections(void * listening_socket_arg) {
	int listenfd = (int *)listening_socket;

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
		exit(1)	
	}

	if (bind(listening_sockfd, servinfo->ai_addr, servinfo->ai_addrlen) < 0) {
		perror("tracker - open_listening_port bind error");
		exit(1);
	}

	freeaddrinfo(servinfo); 	// free filled out structure

	return listening_sockfd;
}