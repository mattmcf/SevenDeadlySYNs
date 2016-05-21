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

#include "network_client.h"


int main() {
	char server_name[1000] = "";

	printf("Enter server to connect to: ");
	fgets(server_name,1000,stdin);

	struct addrinfo hints;
  struct addrinfo *result, *rp;

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
		fprintf("failed to find any matching results.\n");
		return 1;
	}

	char ip6_addr[100] = "";
	inet_ntop(AF_INET6, result->ai_addr, ip6_addr, result->ai_addr);
	printf("Trying to connect to %s\n...", ip6_addr);


	// starts client network
	CNT * cnt = StartClientNetwork(result->ai_addr, result->ai_addr);

	freeaddrinfo(result);

	while (1) {
		printf("main thread sleeping\n");
		sleep(10);
	}

}