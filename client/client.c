/* APP: DartSync	MODULE: client 
 * client.c 	- standalone application to open, monitor, and manage a 
 *				file system that is shared between other peers.  Need to be 
 *				able to initialize a shared folder with a tracker, share it
 *				with peers, receive changes from peers, push changes to peers,
 *				and drop in and out of the network of tracker and peers.
 */

/* ------------------------- System Libraries -------------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

/* -------------------------- Local Libraries -------------------------- */
#include "client.h"
#include "network_client.h"

/* ----------------------------- Constants ----------------------------- */
pthread_t clt_net_t;
FILE *metadata;
FILE *

ClientNetworkThread* cnt;

/* ------------------------- Global Variables -------------------------- */


/* ---------------------- Private Function Headers --------------------- */

/* ----------------------- Public Function Bodies ---------------------- */

int MonitorFilesystem(){

}

int RequestUpdate(){

}

int UpdateClientTable(){

}

int DropFromNetwork(){
	/* call Matt's drop from network function */
	// dropFromNetwork(cnt);

	/* close our files and free our memory */
}

/* ----------------------- Private Function Bodies --------------------- */


/* ------------------------------- main -------------------------------- */
int main(int argv, char* argc[]){

	/* arg check */
	if (2 != argv){
		printf("CLIENT MAIN: need to pass the tracker IP address\n");
		exit(0);
	} 

	/* start the client connection to the tracker and network */
	if (NULL == (cnt = StartClientNetwork(argc[1], argv))){
		printf("CLIENT MAIN: StartClientNetwork failed\n");
		exit(0);
	}

	/* catch sig int so that we can politely close networks on kill */
	signal(SIGINT, dropFromNetwork);

	/* open the metadata, if it exists then you are rejoining the network,
	 * else you are joining for the first time and should make the file and
	 * make a request for every file in the network */
	if (0 == access(DARTSYNC_METADATA, (F_OK))){	// if it exists
		/* open the file */
		if (0 != fopen(DARTSYNC_METADATA, "r")){
			printf("CLIENT MAIN: couldn't open metadata file, 
					probably permission issue\n");
			exit(0);
		}
		
	} else {	// else it doesn't exist

	}

	// Start using cnt and doing things

	// getNextThing(cnt);
}



