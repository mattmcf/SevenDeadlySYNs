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
CNT* cnt;
FileSystem *cur_fs;
peer_table_t *pt;

/* ------------------------------- TODO -------------------------------- */

/* add password recognition */

/* ----------------------------- Questions ----------------------------- */

// should chunk be already malloc'd?  what about chunk size?
// how to destroy a chunky file and a chunk?

/* ---------------------- Private Function Headers --------------------- */

/* check if file system is empty, used to check diffs for anything */
int CheckFileSystem(FileSystem *fs);

/* calloc the peer table and check the return function 
 *		(not claimed) - pt: the peer table
 */
int CreatePeerTable(peer_table_t *pt);

/* calloc a new peer for the table and append it to the list.
 * 		(not claimed) - peer: the new peer to be appended
 */
int InsertPeer(peer_table_t *pt, int peer_id, int status);

/* 
 * find and remove the specified peer id
 * 		(claimed) - peer: the peer specified by peer_id
 */
int RemovePeer(peer_table_t *pt, int peer_id);

/*
 * iterate through the table and destroy each entry, then destroy the table
 * 		(claimed) - pt: the peer table and all its entries
 */
int DestroyPeerTable(peer_table_t *pt);

/* 
 * iterate over a filesystem that is assumed to be the deletions filesystem
 * after a diff and delete any files that it points to
 */
int RemoveFileDeletions(FileSystem *deletions);

/* 
 * iterate over a filesystem that is assumed to be the additions filesystem
 * after a diff and make requests for the files' chunks from peers
 */
int GetFileAdditions(FileSystem *additions);

/* ----------------------- Public Function Bodies ---------------------- */

int SendMasterFSRequest(FileSystem *cur_fs){
	FileSystem *master;

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

	UpdateLocalFilesystem(cur_fs, master);
	return 1;
}

void UpdateLocalFilesystem(FileSystem *cur_fs, FileSystem *new_fs){
	printf("UpdateLocalFilesystem: received update from master!\n");

	/* received the master file system, iterate over it to look for differences */
	FileSystem *additions;
	FileSystem *deletions;
	filesystem_diff(cur_fs, new_fs, &additions, &deletions);

	if (!deletions && (-1 == CheckFileSystem(deletions))){
		printf("UpdateLocalFilesystem: deletions is NULL or empty\n");
	} else {
		/* iterate over additions to see if we need to request files */
		/* delete any files that we need to update, or remove flat out */
		RemoveFileDeletions(deletions);
	}

	if (!additions && (-1 == CheckFileSystem(additions))){
		printf("UpdateLocalFilesystem: additions is NULL\n");
	} else {
		/* iterate over additions to see if we need to request files */
		printf("UpdateLocalFilesystem: ready to iterate over additions!\n");
		FileSystemIterator* add_iterator = filesystemiterator_new(additions);

		if (!add_iterator){
			printf("UpdateLocalFilesystem: failed to make add iterator\n");
		}

		char *path;
		while (NULL != (path = filesystemiterator_next(add_iterator))){
			printf("UpdateLocalFilesystem: found addition at: %s\n", path);

			/* figure out which client to get the file from */

			/* make the requet */

			/* receive the file from the peer */

			/* update local file system with received file */

			/* get ready for next iteration */
			free(path);
			path = NULL;
		}

		filesystemiterator_destroy(add_iterator);
	}

	/* replace the old file system with the new one */
	filesystem_destroy(cur_fs);
	cur_fs = NULL;
	cur_fs = new_fs;

	/* cleanup */
	filesystem_destroy(additions);
	filesystem_destroy(deletions);
}

void CheckLocalFilesystem(FileSystem *cur_fs){
	FileSystem *new_fs = filesystem_new(DARTSYNC_DIR);
	FileSystem *adds = NULL, *dels = NULL;
	filesystem_diff(cur_fs, new_fs, &adds, &dels);

	/* if there are either additions or deletions, then we need to let the 
	 * master know */
	if (!adds && !dels){
		printf("CLIENT MAIN: diff failed\n");
		filesystem_destroy(new_fs);
	} else if ((1 == CheckFileSystem(adds)) || (1 == CheckFileSystem(dels))){
		printf("CLIENT MAIN: about to send diffs to the tracker\n");
		/* send the difs to the tracker */
		if (-1 == send_updated_files(cnt, adds, dels)){
			printf("CLIENT MAIN: send_updated_files() failed\n");
		}

		/* update the pointer to our *current* filesystem */
		filesystem_destroy(cur_fs);
		filesystem_destroy(adds);
		filesystem_destroy(dels);
		cur_fs = NULL;
		cur_fs = new_fs; 
	} else {
		/* else we have no diffs so just destroy the new fs we created */
		filesystem_destroy(new_fs);
		filesystem_destroy(adds);
		filesystem_destroy(dels);
	}
}

void DropFromNetwork(){
	/* call Matt's drop from network function */
	EndClientNetwork(cnt);

	/* close our files and free our memory */
	filesystem_destroy(cur_fs);
	DestroyPeerTable(pt);
}

int RemoveFileDeletions(FileSystem *deletions){
	printf("RemoveFileDeletions: ready to iterate over deletions!\n");
	FileSystemIterator* del_iterator = filesystemiterator_new(deletions);
	char *path;

	if (!del_iterator){
		printf("RemoveFileDeletions: failed to make del iterator\n");
		return -1;
	}

	/* iterate over the fileystem, delete any files that are in it */
	while (NULL != (path = filesystemiterator_next(del_iterator))){
		printf("RemoveFileDeletions: found deletion at: %s\n", path);
		if (-1 == remove(path)){
			printf("RemoveFileDeletions: remove() failed on path\n");
		}

		free(path);
		path = NULL;
	}

	filesystemiterator_destroy(del_iterator);
	return 1;
}

int GetFileAdditions(FileSystem *additions){
	printf("GetFileAdditions: ready to iterate over additions!\n");
	FileSystemIterator* add_iterator = filesystemiterator_new(deletions);
	char *path;

	if (!add_iterator){
		printf("GetFileAdditions: failed to make add iterator\n");
		return -1;
	}

	/* iterate over the fileystem, delete any files that are in it */
	while (NULL != (path = filesystemiterator_next(add_iterator))){
		printf("GetFileAdditions: found addition at: %s\n", path);

		/* figure out who to request from */

		/* make that request */

		free(path);
		path = NULL;
	}

	filesystemiterator_destroy(add_iterator);
	return 1;
}

/* ----------------------- Private Function Bodies --------------------- */

int CreatePeerTable(peer_table_t *pt){
	if (!(pt = calloc(1, sizeof(peer_table_t)))){
		printf("CreatePeerTable: calloc() failed\n");
		return -1;
	}

	return 1;
}

int InsertPeer(peer_table_t *pt, int peer_id, int status){
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

int RemovePeer(peer_table_t *pt, int peer_id){
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

int DestroyPeerTable(peer_table_t *pt){
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
		filesystemiterator_destroy(iterator);
		free(path);
		return 1;
	}
	printf("CheckFileSystem: FileSystem is empty\n");
	filesystemiterator_destroy(iterator);
	return -1;
}

/* ------------------------------- main -------------------------------- */
int main(int argv, char* argc[]){

	/* arg check */
	if (2 != argv){
		printf("CLIENT MAIN: need to pass the tracker IP address\n");
		exit(0);
	} 

	char ip_addr[sizeof(struct in_addr)]; 	// IPv4 address length
	if (inet_pton(AF_INET, argc[1], ip_addr) != 1) {
		fprintf(stderr,"Could not parse host %s\n", argc[1]);
		exit(0);
	}

	/* start the client connection to the tracker and network */
	/* may eventually update to add password sending */
	if (NULL == (cnt = StartClientNetwork(ip_addr, sizeof(struct in_addr)) ) ) {
		printf("CLIENT MAIN: StartClientNetwork failed\n");
		exit(0);
	}

	/* catch sig int so that we can politely close networks on kill */
	signal(SIGINT, DropFromNetwork);
	
	/* check if the folder already exists, if it doesn't then make it */
	if (0 != access(DARTSYNC_DIR, (F_OK))){
		/* it doesn't exist, so make it */
		if (-1 == mkdir(DARTSYNC_DIR, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)){
			printf("CLIENT MAIN: failed to create DARTSYNC_DIR\n");
		}
	} 

	/* get the current local filesystem */
	if (NULL == (cur_fs = filesystem_new(DARTSYNC_DIR))){
		printf("CLIENT MAIN: filesystem_new() failed\n");
		exit(-1);
	}
	filesystem_print(cur_fs);

	/* create a peer table that we will use to keep track of who to request what
	 * file from */
	if (-1 == CreatePeerTable(pt)){
		printf("CLIENT MAIN: CreatePeerTable() failed\n");
	}

	/* send a request to the tracker for the master filesystem. 
	 * this will check against our current filesystem and make requests for updates */
	SendMasterFSRequest(cur_fs);

	/* start the loop process that will run while we are connected to the tracker 
	 * this will handle peer adds and dels and receive updates from the master 
	 * filesystem and monitor the local filesystem */
	int new_client = -1, del_client = -1;
	FileSystem *master;
	int recv_len;
	while (1){
		sleep(POLL_STATUS_DIFF);

		/* get any new clients first */
		while (-1 != (new_client = recv_peer_added(cnt))){
			if (-1 == InsertPeer(pt, new_client, CONN_ACTIVE)){
				printf("CLIENT MAIN: InsertPeer() failed\n");
			}
			new_client = -1;
		}

		/* figure out if any clients have dropped from the network */
		while (-1 != (del_client = recv_peer_deleted(cnt))){
			if (-1 == RemovePeer(pt, del_client)){
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
			ChunkyFile *file = chunkyfile_new_from_path(filepath);
			// should chunk be already malloc'd?  what about chunk size?
			char *chunk_text;
			int chunk_len;
			chunkyfile_get_chunk(file, chunk_id, char** chunk, int* chunkSize);

			/* send that chunk to the peer */
			send_chunk(cnt, peer_id, chunk_text, chunk_len);

			/* destroy that chunky file how? */
		}
		free(peer_id);
		free(chunk_id);
		free(len);

		/* poll for any received chunks */
		char *chunk_data;
		int chunk_id;
		while (NULL != (chunk_data = receive_chunk(cnt))){

		}

		/* poll for any diffs, if they exist then we need to make updates to 
		 * our filesystem */
		FileSystem *additions, *deletions;
		while (-1 != recv_diff(cnt, &additions, &deletions)){
			/* get rid of the deletions */
			RemoveFileDeletions(deletions);

			/* get the file additions from peers, and update the filesystem */
			GetFileAdditions(additions);
		}

		/* poll to see if there are any changes to the master file system 
		 * if there are then we need to get those parts from peers and then
		 * copy master to our local pointer of the filesystem */
		while (NULL != (master = recv_master(cnt, &recv_len))){
			UpdateLocalFilesystem(cur_fs, master);
		}

		/* check the local filesystem for changes that we need to push
		 * to master */
		CheckLocalFilesystem(cur_fs);
	}

}



