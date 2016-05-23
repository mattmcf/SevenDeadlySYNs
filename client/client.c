/* APP: DartSync	MODULE: client 
 * client.c 	- standalone application to open, monitor, and manage a 
 *				file system that is shared between other peers.  Need to be 
 *				able to initialize a shared folder with a tracker, share it
 *				with peers, receive changes from peers, push changes to peers,
 *				and drop in and out of the network of tracker and peers.
 */

/* ------------------------- System Libraries -------------------------- */
#include <stdio.h>		//print
#include <stdlib.h>		//exit
#include <unistd.h>		//mkdir
#include <sys/stat.h>	//mkdir
#include <arpa/inet.h>	// in_addr_t
#include <pthread.h>	//thread stuff

/* -------------------------- Local Libraries -------------------------- */
#include "client.h"
#include "network_client.h"
#include "../utility/FileSystem.h"

/* ----------------------------- Constants ----------------------------- */
#define CONN_ACTIVE 0
#define CONN_CLOSED 1

/* ------------------------- Global Variables -------------------------- */
pthread_t clt_net_t;
FILE *metadata;

CNT* cnt;
FileSystem* fs;

peer_table_t *pt;

/* ---------------------- Private Function Headers --------------------- */

/* calloc the peer table and check the return function 
 *		(not claimed) - pt: the peer table
 */
int CreatePeerTable();

/* calloc a new peer for the table and append it to the list.
 * 		(not claimed) - peer: the new peer to be appended
 */
int InsertPeer(int peer_id, bool status);

/* If we have an active connection with a peer, then we need to update the
 * table information to reflect that */
int ActivatePeer(int peer_id);

/* the connection to peer_id has been deactivated so we need to reflect that
 * in the table */
int DeactivatePeer(int peer_id);

/* 
 * find and remove the specified peer id
 * 		(claimed) - peer: the peer specified by peer_id
 */
int RemovePeer(int peer_id);

/*
 * iterate through the table and destroy each entry, then destroy the table
 * 		(claimed) - pt: the peer table and all its entries
 */
int DestroyPeerTable();

/* ----------------------- Public Function Bodies ---------------------- */

int SendMasterFSRequest(){

}

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

int CreatePeerTable(){
	if (!(pt = calloc(1, sizeof(peer_table_t))){
		printf("CreatePeerTable: calloc() failed\n");
		return -1;
	}

	return 1;
}

int InsertPeer(int peer_id, bool status){
	peer_t *peer = calloc(1, sizeof(peer_t));
	if (!peer){
		printf("InsertPeer: calloc failed\n");
		return -1;
	}

	peer->peer_id = peer_id;
	peer->active = status;

	/* this is the first peer so we need to make it the head and tail */
	if (NULL == pt->head){
		printf("InsertPeer: inserting %d to the head of pt\n", peer_id);
		pt->head = peer;
		pt->tail = peer;
	} else {
		printf("InsertPeer: inserting %d to the tail of pt\n", peer_id);
		pt->tail->next = peer;
		pt->tail = peer;
	}
}

int ActivatePeer(int peer_id){
	peer_t *peer = pt->head;
	if (!peer){
		pritnf("ActivatePeer: pt is empty\n");
		return 1;
	}

	while (NULL != peer){
		if (peer_id == peer->peer_id){
			printf("ActivatePeer: activating peer: %d\n", peer_id);
			peer->status = CONN_ACTIVE;
			return 1;
		}
		peer = peer->next;
	}

	printf("ActivatePeer: couldn't find peer: %d\n", peer_id);
	return -1;
}

int DeactivatePeer(int peer_id){
	peer_t *peer = pt->head;
	if (!peer){
		pritnf("DeactivatePeer: pt is empty\n");
		return 1;
	}

	while (NULL != peer){
		if (peer_id == peer->peer_id){
			printf("DeactivatePeer: deactivating peer: %d\n", peer_id);
			peer->status = CONN_CLOSED;
			return 1;
		}
		peer = peer->next;
	}

	printf("DeactivatePeer: couldn't find peer: %d\n", peer_id);
	return -1;
}

int RemovePeer(int peer_id){
	peer_t *peer = pt->head;
	if (!peer){
		pritnf("RemovePeer: pt is empty\n");
		return 1;
	}

	if (peer_id == peer->peer_id){	// if it's the head
		peer_t *temp = peer;
		pt->head = peer->next;
		free(temp);
		printf("RemovePeer: found the peer at the head of the list\n");
		return 1;
	} else {
		while (NULL != peer->next){
			if (peer_id == peer->next->peer_id){
				peer_t *temp = peer->next;
				peer->next = peer->next->next;
				printf("RemovePeer: found the peer inside the list\n");
				free(temp);
				return 1;
			}
		}
	}

	printf("RemovePeer: didn't find that peer\n");
	return -1;
}

int DestroyPeerTable(){
	printf("DestroyPeerTable: destroying the peer table\n");
	while (pt->head){
		peer_t *temp = pt->head;
		pt->head = pt->head->next;
		free(temp);
	}

	free(pt);
}

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

	sleep(1);

	if (-1 == SendMasterFSRequest()){
		printf("CLIENT MAIN: SendMasterFSRequest() failed\n");
	}

	/* catch sig int so that we can politely close networks on kill */
	signal(SIGINT, DropFromNetwork);

	/* open the metadata, if it exists then you are rejoining the network,
	 * else you are joining for the first time and should make the file and
	 * make a request for every file in the network */
	if (0 == access(DARTSYNC_METADATA, (F_OK))){	// if it exists
		/* open the file */
		if (0 != fopen(DARTSYNC_METADATA, "r")){
			printf("CLIENT MAIN: couldn't open metadata file\n");
			exit(0);
		}
	} else {	// else it doesn't exist
		/* check if the folder already exists, if it doesn't then make it */
		if (0 != access(DARTSYNC_DIR, (F_OK))){
			/* it doesn't exist, so make it */
			if (-1 == mkdir(DARTSYNC_DIR, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)){
				printf("CLIENT MAIN: failed to create DARTSYNC_DIR\n");
			}
		} 
	}

	/* get our current file system and send it to the tracker for diffs */
	if (NULL == (fs = filesystem_new(DARTSYNC_DIR))){
		printf("CLIENT MAIN: filesystem_new failed\n");
	}

	if (-1 == send_status(cnt, fs)){
		printf("CLIENT MAIN: send_status failed\n");
	}

	FileSystem *diff;
	while (1){
		sleep(POLL_STATUS_DIFF);

		if (NULL != (diff = recv_diff(cnt))){
			/* iterate over the filesystem to search for diffs */
			FileSystemIterator *iterator;
			if (NULL == (iterator = filesystemiterator_new(diff))){
				printf("CLIENT MAIN: failed to init fs iterator or no diff\n");
				continue;
			}
			char *path;
			while (NULL != (path = filesystemiterator_next(iterator))){
				/* we have a diff, replace the old path with the current path */
				printf("CLIENT MAIN: found diff at: %s, replacing old with new\n", path);
				FILE *old;
				if (NULL == (old = fopen(path, "w"))){
					printf("CLIENT MAIN: failed to fopen file at %s\n", path);
				}
					/* receive the table of peers with each part of a file */
					/* randomly select a peer for each chuck to decide where to get the chunk */
					/* make a request to that peer to get that chunk */
					/* update that chunk with the received update */

				free(path);
				path = NULL;
			}

			filesystemiterator_destroy(iterator);
		}
	}

	// getNextThing(cnt);
}



