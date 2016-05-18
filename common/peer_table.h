/*
 * peer_table.h
 *
 * defines the peer structure used both by the client and tracker
 *
 */

#ifndef _PEER_TABLE_H
#define _PEER_TABLE_H

#define IP_LEN 16 	// IPv6 address

typedef struct peer {
	char ip_addr[IP_LEN]; 
	int socketfd;
	int id;  
} peer_t;


typedef struct peer_table {
	peer_t ** table;
	int size;
} peer_table_t;

peer_table_t * init_peer_table(int size);

peer_t * add_peer(peer_table_t * table, char * ip_addr);

void delete_peer(peer_table_t * table, int id);

void destroy_table(peer_table_t * table);

void print_table(peer_table_t * table);

#endif // _PEER_TABLE_H