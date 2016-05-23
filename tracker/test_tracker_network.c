/*
 * test_tracker_network.c
 *
 * just spins up the tracker's network thread
 *
 */

#include <stdio.h>
#include <unistd.h>
#include "network_tracker.h"

int main() {

	printf("Starting up network thread\n");

	TNT * tnt = StartTrackerNetwork();
	if (!tnt) {
		fprintf(stderr,"Something went wrong while spinning up the network thread\n");
		return 1;
	}

	FileSystem * master = filesystem_new("/Users/McFarland/dartsync");

	while (1) {
		//printf("main thread sleeping...\n");
		sleep(5);

		int client_id = -1;
		if ((client_id = receive_master_request(tnt)) > 0) {
			printf("Received request for master from client %d -- sending it now!\n",client_id);
			send_master(tnt, client_id, master);
		}
	}
}
