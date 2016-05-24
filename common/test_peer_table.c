// test_peer_table.c
//
// tests peer table

#include "peer_table.h"
#include <stdio.h>

int main() {

	// fill dummy ip
	char dummy_ip[IP_LEN];
	for (int i = 0; i < IP_LEN; i++) {
		dummy_ip[i] = (char)('a' + i);
	}

	peer_table_t * table = init_peer_table(8);
	if (!table) {
		printf("error creating table!\n");
		return 1;
	}

	print_table(table);

	for (int i = 0; i < 15; i++) {
		if (!add_peer(table, dummy_ip, i)) {
			printf("error adding element %d\n", i);
		}
	}

	print_table(table);	

	/* test serializing, deserializing and copying */
	printf("serializing...\n");
	int table_len;
	char * buf = serialize_peer_table(table, &table_len);
	if (!buf || table_len < 0) {
		fprintf(stderr,"serializing error occurred\n");
		return 1;
	}

	printf("deserializing...\n");
	peer_table_t * copy = deserialize_peer_table(buf, table_len);
	if (!copy) {
		fprintf(stderr, "deserializing error occurred\n");
		return 1;
	}

	printf("copy below\n");
	print_table(copy);

	char copy_ip_addr[IP_LEN] = "123";
	for (int i = 16; i < 25; i++) {
		copy_ip_addr[IP_LEN - 1] = (char)i;
		copy_peer(copy, i, copy_ip_addr, i);
	}

	printf("\nadded some elements to copy table\n");
	print_table(copy);

	for (int i = 0; i < 5; i++) {
		delete_peer(copy, i);
	}

	printf("\ndeleted some elements of copy table\n");
	print_table(copy);

	printf("Diffing tables\n");
	/* compare differences between original table and copied with additions and deletions */
	peer_table_t * additions = NULL;
	peer_table_t * deletions = NULL;
	if (diff_tables(table, copy, &additions, &deletions) != 1) {
		fprintf(stderr, "diff tables failed\n");
		return 1;
	}

	printf("Additions:\n");
	print_table(additions);

	printf("Deletions:\n");
	print_table(deletions);

	if (!get_peer_by_id(copy, 24)) {
		printf("couldn't find copy's id 24\n");
		return 1;
	}

	if (!get_peer_by_socket(table, 14)) {
		printf("couldn't find table's socket 14\n");
		return 1;
	}

	char find_ip_addr[4] = "123";
	find_ip_addr[3] = (char)16;

	if (!get_peer_by_ip(copy, find_ip_addr)) {
		printf("couldn't find copy's peer by ip\n");
		return 1;
	}

	find_ip_addr[0] = (char)0;
	if (get_peer_by_ip(copy, find_ip_addr)) {
		printf("found extraneous peer\n");
		return 1;
	}

	/* delete all entries */

	for (int i = 0; i < 16; i++) {
		delete_peer(table, i);
	}



	printf("Deleted all entries -- copy should be empty\n");
	print_table(table);

	destroy_table(additions);
	destroy_table(deletions);
	destroy_table(table);
	destroy_table(copy);

}
