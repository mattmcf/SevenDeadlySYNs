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
#include "../utility/FileSystem/FileSystem.h"
#include "../common/constant.h"

/* ----------------------------- Constants ----------------------------- */
#define CONN_ACTIVE 0
#define CONN_CLOSED 1

/* ------------------------- Global Variables -------------------------- */
FILE *metadata;
FileSystem *cur_fs;
CNT* cnt;
peer_table_t *pt;

/* ------------------------------- TODO -------------------------------- */

/* add password recognition */

/* ----------------------------- Questions ----------------------------- */

/* is a deletion only a total file deletion or can it simply imply a 
 * chunk was deleted */

/* ---------------------- Private Function Headers --------------------- */

/* check if file system is empty, used to check diffs for anything */
int CheckFileSystem(FileSystem *fs);

/* calloc the peer table and check the return function 
 *		(not claimed) - pt: the peer table
 */
int CreatePeerTable();

/* calloc a new peer for the table and append it to the list.
 * 		(not claimed) - peer: the new peer to be appended
 */
int InsertPeer(int peer_id, int status);

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
	FileSystem *master;
	char *path;

	if (-1 == send_status(cnt, cur_fs)){
		printf("SendMasterFSRequest: send_status failed\n");
		return -1;
	}

	if (-1 == send_request_for_master(cnt)){
		printf("SendMasterFSRequest: send_request_for_master() failed\n");
		return -1;
	}

	/* get the master file system and check to see if there are differences */
	int master_len = 0;
	while (NULL == (master = recv_master(cnt, &master_len))){
		printf("SendMasterFSRequest: recv_master returned NULL, sleeping\n");
		sleep(1);
	}

	/* received the master file system, iterate over it to look for differences */
	FileSystem *additions;
	FileSystem *deletions;
	filesystem_diff(cur_fs, master, &additions, &deletions);

	if (!additions && (-1 == CheckFileSystem(additions))){
		printf("SendMasterFSRequest: additions is NULL\n");
	} else {
		/* iterate over additions to see if we need to request files */
		printf("SendMasterFSRequest: ready to iterate over additions!\n");
		FileSystemIterator* add_iterator = filesystemiterator_new(additions);

		if (!add_iterator){
			printf("SendMasterFSRequest: failed to make add iterator\n");
		}

		while (NULL != (path = filesystemiterator_next(add_iterator))){
			printf("SendMasterFSRequest: found addition at: %s\n", path);

			/* figure out which client to get the file from */

			/* make the requet */

			/* receive the file from the peer */

			/* update local file system with received file */
		}
	}

	if (!deletions && (-1 == CheckFileSystem(deletions))){
		printf("SendMasterFSRequest: deletions is NULL or empty\n");
	} else {
		/* iterate over additions to see if we need to request files */
		printf("SendMasterFSRequest: ready to iterate over deletions!\n");
		FileSystemIterator* del_iterator = filesystemiterator_new(deletions);

		if (!del_iterator){
			printf("SendMasterFSRequest: failed to make add iterator\n");
		}

		while (NULL != (path = filesystemiterator_next(del_iterator))){
			printf("SendMasterFSRequest: found addition at: %s\n", path);
		}
	}

	return 1;
}

int MonitorFilesystem(){

	return 1;
}

int RequestUpdate(){
	return 1;
}

int UpdateClientTable(){
	return 1;
}

void DropFromNetwork(){
	/* call Matt's drop from network function */
	printf("DropFromNetwork: about to free matt's memory\n");
	EndClientNetwork(cnt);

	/* close our files and free our memory */
	printf("DropFromNetwork: Matt's memory is free, now for mine\n");
	filesystem_destroy(cur_fs);
	DestroyPeerTable();
}

/* ----------------------- Private Function Bodies --------------------- */

int CreatePeerTable(){
	if (!(pt = calloc(1, sizeof(peer_table_t)))){
		printf("CreatePeerTable: calloc() failed\n");
		return -1;
	}

	return 1;
}

int InsertPeer(int peer_id, int status){
	peer_t *peer = calloc(1, sizeof(peer_t));
	if (!peer){
		printf("InsertPeer: calloc failed\n");
		return -1;
	}

	peer->peer_id = peer_id;
	peer->status = status;

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

	return 1;
}

int ActivatePeer(int peer_id){
	peer_t *peer = pt->head;
	if (!peer){
		printf("ActivatePeer: pt is empty\n");
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
		printf("DeactivatePeer: pt is empty\n");
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
		printf("RemovePeer: pt is empty\n");
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
	return 1;
}

int CheckFileSystem(FileSystem *fs){
	char *path;
	FileSystemIterator *iterator = filesystemiterator_new(fs);
	if (NULL != (path = filesystemiterator_next(iterator))){
		printf("CheckFileSystem: FileSystem is nonempty\n");
		free(path);
		return 1;
	}
	printf("CheckFileSystem: FileSystem is empty\n");
	return -1;
}

/* ------------------------------- main -------------------------------- */
int main(int argv, char* argc[]){

	/* arg check */
	if (2 != argv){
		printf("CLIENT MAIN: need to pass the tracker IP address\n");
		exit(0);
	} 

	/* start the client connection to the tracker and network */
	/* may eventually update to add password sending */
	if (NULL == (cnt = StartClientNetwork(argc[1], argv))){
		printf("CLIENT MAIN: StartClientNetwork failed\n");
		exit(0);
	}

	/* catch sig int so that we can politely close networks on kill */
	signal(SIGINT, DropFromNetwork);

	/* open the metadata, if it exists then you are rejoining the network,
	 * else you are joining for the first time and should make the file and
	 * make a request for every file in the network */
	//if (0 == access(DARTSYNC_METADATA, (F_OK))){	// if it exists
	//	/* open the file */
	//	if (0 != fopen(DARTSYNC_METADATA, "r")){
	//		printf("CLIENT MAIN: couldn't open metadata file\n");
	//		exit(0);
	//	}
	//} else {	// else it doesn't exist
	//	/* check if the folder already exists, if it doesn't then make it */
	//	if (0 != access(DARTSYNC_DIR, (F_OK))){
	//		/* it doesn't exist, so make it */
	//		if (-1 == mkdir(DARTSYNC_DIR, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)){
	//			printf("CLIENT MAIN: failed to create DARTSYNC_DIR\n");
	//		}
	//	} 
	//}

	if (NULL == (cur_fs = filesystem_new(DARTSYNC_DIR))){
		printf("CLIENT MAIN: filesystem_new() failed\n");
		exit(-1);
	}

	if (-1 == CreatePeerTable()){
		printf("CLIENT MAIN: CreatePeerTable() failed\n");

	}

	SendMasterFSRequest();

	int new_client = -1, del_client = -1;
	FileSystem *master;
	int recv_len;
	while (1){
		sleep(POLL_STATUS_DIFF);

		/* get any new clients first */
		while (-1 != (new_client = recv_peer_added(cnt))){
			if (-1 == InsertPeer(new_client, CONN_ACTIVE)){
				printf("CLIENT MAIN: InsertPeer() failed\n");
			}
			new_client = -1;
		}

		/* figure out if any clients have dropped from the network */
		while (-1 != (del_client = recv_peer_deleted(cnt))){
			if (-1 == RemovePeer(del_client)){
				printf("CLIENT MAIN: RemovePeer() failed\n");
			}
			del_client = -1;
		}

		/* check to see if any requests for files have been made to you */
		int *peer_id = malloc(sizeof(int));
		int *chunk_id = malloc(sizeof(int));
		int *len = malloc(sizeof(int));
		char *filepath;
		while (-1 != receive_chunk_request(cnt, &filepath, peer_id, chunk_id, len)){
			printf("CLIENT MAIN: received chunk request from peer: %d\n", *peer_id);

			/* get the chunk that they are requesting */

			/* send that chunk to the peer */

		}
		free(peer_id);
		free(chunk_id);
		free(len);

		/* poll to see if there are any changes to the master file system 
		 * if there are then we need to get those parts from peers and then
		 * copy master to our local pointer of the filesystem */
		while (NULL != (master = recv_master(cnt, &recv_len))){
			printf("CLIENT MAIN: received update from master!\n");

			/* figure out which peer to make the request to */

			/* figure out which chunk to request? */

			/* make the request to that peer */
		}

		/* check the local filesystem for changes that we need to push
		 * to master */
		FileSystem *new_fs = filesystem_new(DARTSYNC_DIR);
		if (NULL == new_fs){
			printf("CLIENT MAIN: new_fs filesystem_new() failed\n");
		}
		FileSystem *adds = NULL, *dels = NULL;
		filesystem_diff(cur_fs, new_fs, &adds, &dels);

		/* if there are either additions or deletions, then we need to let the 
		 * master know */
		if (!adds && !dels){
			printf("CLIENT MAIN: diff failed\n");
		} else if ((1 == CheckFileSystem(adds)) || (1 == CheckFileSystem(dels))){
			printf("CLIENT MAIN: about to send diffs to the tracker\n");
			/* send the difs to the tracker */

			if (-1 == send_updated_files(cnt, adds, dels)){
				printf("CLIENT MAIN: send_updated_files() failed\n");
			}
		}
	}

}



