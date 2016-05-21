/*
 * test_network_client.h
 *
 * Spins up the basic network thread to connect to the tracker
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>

#include "network_client.h"
#include "../common/constant.h"

int main() {
	char server_name[1000] = "";

	printf("Enter server to connect to: ");
	fgets(server_name,1000,stdin);

	struct addrinfo hints;
  struct addrinfo *result, *rp;

  struct sockaddr_in6 servaddr;

  memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET6;    /* IPv6 */
	hints.ai_socktype = SOCK_STREAM; /* tcp socket */
	hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
	hints.ai_protocol = 6;          /* tcp protocol */
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

	printf("Trying to find serverinfo at: %s\n", server_name);
	getaddrinfo(server_name, NULL, &hints, &result);

	if (!result) {
		fprintf(stderr,"failed to find any matching results\n");
		return 1;
	}

	memcpy(&servaddr, result->ai_addr, result->ai_addrlen);
	servaddr.sin6_port = htons(TRACKER_LISTENING_PORT);
	servaddr.sin6_family = AF_INET6;

	char ip_addr[100] = "";
	inet_ntop(result->ai_family, &servaddr.sin6_addr, ip_addr, 100);
	printf("Trying to connect to %s\n...", ip_addr);

	// starts client network
	CNT * cnt = StartClientNetwork((char *)&servaddr.sin6_addr, 16);

	freeaddrinfo(result);

	while (1) {
		printf("main thread sleeping\n");
		sleep(10);
	}

}
