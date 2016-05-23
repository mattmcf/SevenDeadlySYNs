// test_peer_table.c
//
// tests peer table

#include "peer_table.h"
#include <stdio.h>

int main() {

	// fill dummy ip
	char dummy_ip[16];
	for (int i = 0; i < 16; i++) {
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

	char copy_ip_addr[16] = "1234567890abcde";
	for (int i = 16; i < 25; i++) {
		copy_ip_addr[15] = (char)i;
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
