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

	printf("Enter IPv6 address or server name to connect to: ");
	fgets(server_name,1000,stdin);

	struct addrinfo hints;
  struct addrinfo *result, *rp;

  	struct sockaddr_in servaddr;
  	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(TRACKER_LISTENING_PORT);
	// servaddr.sin_addr = inet_addr(ip_addr);
	


 //  struct sockaddr_in6 servaddr;

 //  memset(&hints, 0, sizeof(struct addrinfo));
	// hints.ai_family = AF_INET6;    /* IPv6 */
	// hints.ai_socktype = SOCK_STREAM; /* tcp socket */
	// //hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
	// hints.ai_protocol = 6;          /* tcp protocol */
	// //hints.ai_canonname = NULL;
	// //hints.ai_addr = NULL;
	// //hints.ai_next = NULL;

	server_name[strlen(server_name)-1] = '\0'; 	// null out new line
	inet_pton(AF_INET, server_name, &servaddr.sin_addr);

	printf("Trying to find serverinfo at: %s\n", server_name);
	// if (getaddrinfo(server_name, NULL, &hints, &result) != 0) {
	// 	perror("getaddrinfo failed: ");
	// 	return 1;
	// }

	// if (!result) {
	// 	fprintf(stderr,"failed to find any matching results\n");
	// 	return 1;
	// }

	// memcpy(&servaddr, result->ai_addr, result->ai_addrlen);
	// servaddr.sin6_port = htons(TRACKER_LISTENING_PORT);
	// servaddr.sin6_family = AF_INET6;

	// char ip_addr[100] = "";
	// inet_ntop(result->ai_family, &servaddr.sin6_addr, ip_addr, 100);
	// printf("Trying to connect to %s...\n", ip_addr);
	printf("Trying to connect to server\n");
	// starts client network
	CNT * cnt = StartClientNetwork((char *)&servaddr.sin_addr, 4);

	// freeaddrinfo(result);

	send_request_for_master(cnt);
	FileSystem * master = NULL;
	int master_len;
	while (1) {
		
		sleep(5);
		master = recv_master(cnt, &master_len);
		if (master) {
			printf("\nClient received master (%d bytes)!\n",master_len);
			fflush(stdout);
			filesystem_print(master);
		}
	}

}
