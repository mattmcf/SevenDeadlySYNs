/*
 * peer_table.c
 *
 * functions for interacting with peer_table
 *
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
#include <time.h>

#include "peer_table.h"

peer_table_t * init_peer_table(int size) {
	if (size <= 0)
		return NULL;

	peer_table_t * table = (peer_table_t *)calloc(1,sizeof(peer_table_t));
	if (!table)
		return NULL;

	table->peer_list = (peer_t **)calloc(size,sizeof(peer_t *));
	if (!table->peer_list)
		return NULL;

	table->size = size;
	table->count = 0;
	table->id_counter = 0;

	return table;	
}

peer_t * add_peer(peer_table_t * table, char * ip_addr, int socketfd) {
	if (!table || !ip_addr)
		return NULL;

	peer_t * new_entry = NULL;
	for (int i = 0; i < table->size; i++) {
		if (!table->peer_list[i]) {

			table->peer_list[i] = (peer_t *)calloc(1,sizeof(peer_t));
			if (!table->peer_list[i])
				return NULL;

			// add entry here
			table->peer_list[i]->id = table->id_counter++;
			memcpy(table->peer_list[i]->ip_addr, ip_addr, IP_LEN);
			table->peer_list[i]->socketfd = socketfd;
			table->peer_list[i]->time_last_alive = time(NULL);

			new_entry = table->peer_list[i]; 	// return entry
			table->count++;
			break;
		}
	}

	if (table->count == table->size) {
		table->size *= 2;
		table->peer_list = (peer_t **)realloc(table->peer_list, table->size * sizeof(peer_t *));
	}

	return new_entry;
}

void delete_peer(peer_table_t * table, int id) {
	if (!table)
		return;

	for (int i = 0; i < table->size; i++) {
		if (table->peer_list[i] && table->peer_list[i]->id == id) {
			free(table->peer_list[i]);
			table->peer_list[i] = NULL;
			table->count--;
			break;
		}
	}
}

void destroy_table(peer_table_t * table) {
	if (!table)
		return;

	for (int i = 0; i < table->size; i++) {
		if (table->peer_list[i])
			free(table->peer_list[i]);
	}
	free(table->peer_list);

	free(table);
}

void print_table(peer_table_t * table) {
	if (!table)
		return;

	char ip_str[INET6_ADDRSTRLEN] = "", time_str[100] = "";

	printf("Printing Peer Table...\n");
	for (int i = 0; i < table->size; i++) {
		if (table->peer_list[i]) {

			inet_ntop(AF_INET6, table->peer_list[i]->ip_addr, ip_str, INET6_ADDRSTRLEN);
			strftime(time_str, 100, "%Y-%m-%d %H:%M:%S", localtime(&table->peer_list[i]->time_last_alive));

			printf("Peer ID: %d, socketfd %d, IP address: %s, last alive %s\n",
				table->peer_list[i]->id, table->peer_list[i]->socketfd, ip_str, time_str);
		}
			
	}
	printf("-- end\n\n");
}
