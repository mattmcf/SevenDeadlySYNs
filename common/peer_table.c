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
	table->id_counter = 1;

	return table;	
}

// should only be used on the tracker side to add entries
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

// should only be used on the client side of the peer table to add entries
peer_t * copy_peer(peer_table_t * table, int id, char * ip_addr, int socketfd) {

	if (!table)
		return NULL;

	for (int i = 0; i < table->size; i++) {
		if (table->peer_list[i] == NULL) {

			table->peer_list[i] = (peer_t *)calloc(1,sizeof(peer_t));
			if (!table->peer_list[i])
				return NULL;

			table->peer_list[i]->id = id;
			memcpy(table->peer_list[i]->ip_addr, ip_addr, IP_LEN);
			table->peer_list[i]->socketfd = socketfd;

			table->count++;
			if (table->count == table->size) {
				table->size *= 2;
				table->peer_list = (peer_t **)realloc(table->peer_list, table->size * sizeof(peer_t *));
			}

			return table->peer_list[i];
		}
	}

	// shouldn't really ever be reached
	return NULL;
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

peer_t * get_peer_by_id(peer_table_t * table, int id) {
	if (!table || id < 1)
		return NULL;

	for (int i = 0; i < table->size; i++) {
		if (table->peer_list[i] && table->peer_list[i]->id == id)
			return table->peer_list[i];
	}

	return NULL;
}

peer_t * get_peer_by_socket(peer_table_t * table, int fd) {
	if (!table || fd < 0)
		return NULL;

	for (int i = 0; i < table->size; i++) {
		if (table->peer_list[i] && table->peer_list[i]->socketfd == fd)
			return table->peer_list[i];
	}

	return NULL;
}

// returns a serialized version of peer table for clients to use
// 	table : (not claimed) table to copy
// 	len : (not claimed) will fill in with # of bytes spit out
// 	ret : (not claimed) data buffer that will hold serialized table copy (NULL on error)
char * serialize_peer_table(peer_table_t * table, int * len) {
	if (!table || !len)
		return NULL;

	if (table->count == 0) {
		*len = 0;
		return NULL;
	}

	int client_size = sizeof(int) + IP_LEN;
	char * buf = (char *)calloc(table->count, client_size);
	char * ptr = buf;

	int written = 0;
	for (int i = 0; i < table->size; i++) {
		if (table->peer_list[i]) {
			memcpy(ptr, &table->peer_list[i]->id, sizeof(int));
			memcpy(ptr + sizeof(int), table->peer_list[i]->ip_addr, IP_LEN);
			ptr += client_size;
			written += client_size;
		}
	} 

	*len = written;

	return buf;
}

// turns data buffer of serialized peer table and creates a copy peer table
// 	buffer : (claimed) serialized data
// 	length : (static) length of buffer
// 	ret : (not claimed) new copy of peer table (NULL on error)
peer_table_t * deserialize_peer_table(char * buffer, int length) {

	if (!buffer || length < 0)
		return NULL;

	if (length == 0)
		return init_peer_table(10); 	// empty table

	int client_size = sizeof(int) + IP_LEN;
	peer_table_t * new_table = init_peer_table(length / client_size * 2);

	char * ptr = buffer;
	for (int i = 0; i < length / client_size; i++) {
		copy_peer(new_table, *(int *)ptr, (char *)(ptr + sizeof(int)), -1);
		new_table->count++;
		ptr += client_size;
	}

	free(buffer);

	return new_table;
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

// from the original table to the new table, creates two new peer_tables,
// 	the additions table contains all of the entries in the new table that aren't in the original
//	the deletions table contains all of the entires in the original that aren't in the new
// 	returns 1 on success, -1 on failure
//	
//	orig : (not claimed) original table
//	new : (not claimed) new table
//	additions : (not claimed) contains all new updates
//	deletions : (not claimed) contains all deleted entries
int diff_tables(peer_table_t * orig, peer_table_t * new, peer_table_t ** additions, peer_table_t ** deletions) {
	if (!orig || !new)
		return -1;

	peer_table_t * added_table, * deleted_table;
	added_table = init_peer_table(new->size);
	deleted_table = init_peer_table(new->size);

	// calculate additions
	for (int i = 0; i < new->size; i++) {
		if (new->peer_list[i]) {

			// if original doesn't contain entry, get_peer_by_id() returns NULL -> added to added table
			if (!get_peer_by_id(orig, new->peer_list[i]->id)) {
				copy_peer(added_table, new->peer_list[i]->id, new->peer_list[i]->ip_addr, new->peer_list[i]->socketfd);
			}
		}
	} 	// end addition difference

	// calculate deletions
	for (int i = 0; i < orig->size; i++) {
		if (orig->peer_list[i]) {

			printf("testing id %d\n",orig->peer_list[i]->id);
			// if new doesn't contain entry, get_peer_by_id() returns NULL -> added to deleted table
			if (!get_peer_by_id(new, orig->peer_list[i]->id)) {
				copy_peer(deleted_table, orig->peer_list[i]->id, orig->peer_list[i]->ip_addr, orig->peer_list[i]->socketfd);
			}
		}
	} 	// end deletion difference

	*additions = added_table;
	*deletions = deleted_table;

	return 1;
}

void print_table(peer_table_t * table) {
	if (!table)
		return;

	char ip_str[INET6_ADDRSTRLEN] = "", time_str[100] = "";

	printf("Printing Peer Table...\n");
	for (int i = 0; i < table->size; i++) {
		if (table->peer_list[i]) {

			printf("%ld %d %ld %ld %ld %s\n", (long)AF_INET6, i, (long)table, (long)table->peer_list, (long)&(table->peer_list[i]->ip_addr), table->peer_list[i]->ip_addr);
			inet_ntop(AF_INET6, table->peer_list[i]->ip_addr, ip_str, INET6_ADDRSTRLEN);
			strftime(time_str, 100, "%Y-%m-%d %H:%M:%S", localtime(&table->peer_list[i]->time_last_alive));

			printf("Peer ID: %d, socketfd %d, IP address: %s, last alive %s\n",
				table->peer_list[i]->id, table->peer_list[i]->socketfd, ip_str, time_str);
		}
			
	}
	printf("-- end\n\n");
}
