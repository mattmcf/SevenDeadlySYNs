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

	while (1) {
		//printf("main thread sleeping...\n");
		sleep(10);
	}
}
