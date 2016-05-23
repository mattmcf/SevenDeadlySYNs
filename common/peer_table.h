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

// should only be used on the tracker side to add entries
peer_t * add_peer(peer_table_t * table, char * ip_addr, int socketfd);

// should only be used on the client side of the peer table to add entries
peer_t * copy_peer(peer_table_t * table, int id, char * ip_addr, int socketfd);

void delete_peer(peer_table_t * table, int id);

peer_t * get_peer_by_id(peer_table_t * table, int id);

peer_t * get_peer_by_socket(peer_table_t * table, int fd);

// returns 
char * serialize_peer_table(peer_table_t * table, int * len);

// claims buffer
peer_table_t * deserialize_peer_table(char * buffer, int length);

// from the original table to the new table, creates two new peer_tables,
// 	the additions table contains all of the entries in the new table that aren't in the original
//	the deletions table contains all of the entires in the original that aren't in the new
// 	returns 1 on success, -1 on failure
//	
//	orig : (not claimed) original table
//	new : (not claimed) new table
//	additions : (not claimed) contains all new updates
//	deletions : (not claimed) contains all deleted entries
int diff_tables(peer_table_t * orig, peer_table_t * new, peer_table_t ** additions, peer_table_t ** deletions);

void destroy_table(peer_table_t * table);

void print_table(peer_table_t * table);

#endif // _PEER_TABLE_H

