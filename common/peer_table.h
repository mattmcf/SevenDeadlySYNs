/*
 * peer_table.h
 *
 * defines the peer structure used both by the client and tracker
 *
 */

#ifndef _PEER_TABLE_H
#define _PEER_TABLE_H

#include <time.h>

#define ALL_PEERS 999
#define IP_LEN 16 	// IPv6 address
#define INITIAL_SIZE 10

typedef struct peer {
	char ip_addr[IP_LEN];	// IP address of client
	int socketfd; 	// -1 if there's no open connection
	int id;  
	time_t time_last_alive;
} peer_t;


typedef struct peer_table {
	peer_t ** peer_list;
	int count;
	int size;
	int id_counter;
} peer_table_t;

peer_table_t * init_peer_table(int size);

peer_t * add_peer(peer_table_t * table, struct in6_addr * ip_addr, int socketfd);

void delete_peer(peer_table_t * table, int id);

peer_t * get_peer_by_id(peer_table_t * table, int id);

peer_t * get_peer_by_socket(peer_table_t * table, int fd);

void destroy_table(peer_table_t * table);

void print_table(peer_table_t * table);

#endif // _PEER_TABLE_H

