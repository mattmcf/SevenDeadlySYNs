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

#include "peer_table.h"

peer_table_t * init_peer_table(int size) {
	if (size <= 0)
		return NULL:

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

peer_t * add_peer(peer_table_t * table, char * ip_addr) {
	if (!table || !ip_addr)
		return NULL:

	for (int i = 0; i < table->size; i++) {
		if (!table->peer_list[i]) {
			table->peer_list[i] = (peer_t *)calloc(1,sizeof(peer_t));
			if (!table->peer_list[i])
				return NULL:


		}
	}
}

void delete_peer(peer_table_t * table, int id);

void destroy_table(peer_table_t * table);

void print_table(peer_table_t * table);
