// test_peer_table.c
//
// tests peer table

#include "peer_table.h"
#include <stdio.h>

int main() {
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

	for (int i = 0; i < 16; i++) {
		delete_peer(table, i);
	}

	print_table(table);

	destroy_table(table);

}
