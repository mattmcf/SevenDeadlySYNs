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
#include <signal.h>
#include <wordexp.h> // for shell expansion of ~
#include <string.h> // strdup

/* -------------------------- Local Libraries -------------------------- */
#include "client.h"
#include "network_client.h"
#include "../utility/FileSystem/FileSystem.h"
#include "../utility/ChunkyFile/ChunkyFile.h"
#include "../utility/FileTable/FileTable.h"
#include "../common/constant.h"

/* ----------------------------- Constants ----------------------------- */
#define CONN_ACTIVE 0
#define CONN_CLOSED 1
#define GET_ALL_CHUNKS 99999

/* ------------------------- Global Variables -------------------------- */
CNT* cnt;
FileSystem *cur_fs;
peer_table_t *pt;
FileTable *ft;
char * dartsync_dir; 	// global of absolute dartsync_dir path

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
int CreatePeerTable();

/* calloc a new peer for the table and append it to the list.
 * 		(not claimed) - peer: the new peer to be appended
 */
int InsertPeer(int peer_id, int status);

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

/* 
 * iterate over a filesystem that is assumed to be the deletions filesystem
 * after a diff and delete any files that it points to
 */
int RemoveFileDeletions(FileSystem *deletions);

/* 
 * iterate over a filesystem that is assumed to be the additions filesystem
 * after a diff and make requests for the files' chunks from peers
 */
int GetFileAdditions(FileSystem *additions, int author_id);

// expands the ~/dart_sync dir
//	original_path : (not claimed) DARTSYNC_DIR
//	ret : (not claimed) expanded path (allocated string on heap)
char * tilde_expand(char * original_path) {
	if (!original_path)
		return NULL;

	printf("expanding string %s\n", original_path);

	char * expanded_string = NULL;
	wordexp_t exp_result;
	wordexp(original_path, &exp_result, 0);
  expanded_string = strdup(exp_result.we_wordv[0]);
  wordfree(&exp_result);

  printf("expanded_string: %s\n", expanded_string);
  return expanded_string;
}

/* ----------------------- Public Function Bodies ---------------------- */

int SendMasterFSRequest(FileSystem *cur_fs){
	FileSystem *master;

	// if (-1 == send_status(cnt, cur_fs)){
	// 	printf("SendMasterFSRequest: send_status failed\n");
	// 	return -1;
	// }

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

	printf("SendMasterFSRequest: dartsync dir - %s\n", dartsync_dir);

	/* set root of returned file system */
	filesystem_set_root_path(master, dartsync_dir);

	UpdateLocalFilesystem(master);
	return 1;
}

void UpdateLocalFilesystem(FileSystem *new_fs){
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
		int len;
		while (NULL != (path = filesystemiterator_next(add_iterator, &len))){
			printf("UpdateLocalFilesystem: found addition at: %s\n", path);

			/* if addition is just a directory -> make that and then go onto files */
			if (len == -1) {

				if (-1 == mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)){
				// char new_dir[1024];
				// snprintf(new_dir, sizeof(new_dir), "mkdir %s", path);
				// if (system(new_dir) != 0) {
					printf("GetFileAdditions: failed to create directory \'%s\'\n", path);
					perror("Failed because");
				}
				continue;
			}

			/* re make that deleted file */
			ChunkyFile *file = chunkyfile_new_empty(len);
			if (!file){
				printf("UpdateLocalFilesystem: chunkyfile_new_empty() failed()\n");
				continue;
			}

			/* save that file to the path */
			chunkyfile_write_to_path(file, path);

			/* figure out how many chunks we need to request */
			int num_chunks = chunkyfile_num_chunks(file);
			if (-1 == num_chunks){
				printf("UpdateLocalFilesystem: chunkyfile_num_chunks() failed\n");
			}

			/* for each chunk, find a peer who has it, and request it */
			for (int i = 0; i < num_chunks; i++){ //???
				printf("UpdateLocalFilesystem: need to request a chunk\n");
			}

			/* get ready for next iteration */
			chunkyfile_destroy(file);
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

void CheckLocalFilesystem(){
	FileSystem *new_fs = filesystem_new(dartsync_dir);
	FileSystem *adds = NULL, *dels = NULL;

	if (!new_fs){
		printf("CheckLocalFilesystem: filesystem_new() failed\n");
		return;
	}

	//printf("CheckLocalFilesystem: cur_fs ------------\n");
	//filesystem_print(cur_fs);
	//printf("CheckLocalFilesystem: new_fs ------------\n");
	//filesystem_print(new_fs);
	filesystem_diff(cur_fs, new_fs, &adds, &dels);
	//printf("CheckLocalFilesystem: got diff\n");

	if (1 == CheckFileSystem(adds)) {
		printf("CheckLocalFilesystem: Additions Seen ----\n");
		filesystem_print(adds);
	}

	if (1 == CheckLocalFilesystem(dels)) {
		printf("CheckLocalFilesystem: Deletions Seen -----\n");
		filesystem_print(dels);
	}

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
	DestroyPeerTable();
	filetable_destroy(ft);

	free(dartsync_dir);

	exit(0);
}

int RemoveFileDeletions(FileSystem *deletions){
	printf("RemoveFileDeletions: ready to iterate over deletions!\n");
	FileSystemIterator* del_iterator = filesystemiterator_new(deletions);
	char *path;
	int len;

	if (!del_iterator){
		printf("RemoveFileDeletions: failed to make del iterator\n");
		return -1;
	}

	/* iterate over the fileystem, delete any files that are in it */
	while (NULL != (path = filesystemiterator_next(del_iterator, &len))){
		printf("RemoveFileDeletions: found deletion at: %s\n", path);
		if (-1 == remove(path)){
			printf("RemoveFileDeletions: remove() failed on path\n");
		}

		path = NULL;
	}

	filesystemiterator_destroy(del_iterator);
	return 1;
}

int GetFileAdditions(FileSystem *additions, int author_id){
	printf("GetFileAdditions: ready to iterate over additions!\n");
	FileSystemIterator* add_iterator = filesystemiterator_new(additions);
	char *path;

	if (!add_iterator){
		printf("GetFileAdditions: failed to make add iterator\n");
		return -1;
	}

	/* iterate over the fileystem, delete any files that are in it */
	int len;
	while (NULL != (path = filesystemiterator_next(add_iterator, &len))){
		printf("GetFileAdditions: found addition at: %s\n", path);

		/* if this is a folder */
		if (-1 == len){
			if (-1 == mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)){
				printf("GetFileAdditions: failed to create directory \'%s\'\n", path);
				perror("Failed because");
			}
			continue;
		}

		/* open chunk file and get the number of chunks */
		printf("Opening new chunky file\n");
		ChunkyFile* file = chunkyfile_new_empty(len);

		/* write that file to the path */
		printf("chunky file write to path: %s\n", path);
		chunkyfile_write_to_path(file, path);

		/* request all chunks */
		printf("Send chunk request\n");
		send_chunk_request(cnt, author_id, path, GET_ALL_CHUNKS);

		/* destroy the chunky file */
		printf("destroy chunky file\n");
		chunkyfile_destroy(file);

		path = NULL;
	}

	filesystemiterator_destroy(add_iterator);
	return 1;
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
	printf("InsertPeer: inserting peer %d\n", peer_id);
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

int RemovePeer(int peer_id){
	printf("RemovePeer: removing peer: %d\n", peer_id);
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
	int len;
	FileSystemIterator *iterator = filesystemiterator_new(fs);
	if (NULL != (path = filesystemiterator_next(iterator, &len))){
		//printf("CheckFileSystem: FileSystem is nonempty\n");
		filesystemiterator_destroy(iterator);
		return 1;
	}
	//printf("CheckFileSystem: FileSystem is empty\n");
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
		exit(-1);
	}

	/* create a peer table that we will use to keep track of who to request what
	 * file from */
	if (-1 == CreatePeerTable()){
		printf("CLIENT MAIN: CreatePeerTable() failed\n");
		exit(-1);
	}

	/* create a filetable that we use to request files from peers */
	if (NULL == (ft = filetable_new())){
		printf("CLIENT MAIN: filetable_new() failed\n");
		DestroyPeerTable();
		exit(-1);
	}

	/* start the client connection to the tracker and network */
	/* may eventually update to add password sending */
	if (NULL == (cnt = StartClientNetwork(ip_addr, sizeof(struct in_addr)) ) ) {
		printf("CLIENT MAIN: StartClientNetwork failed\n");
		exit(-1);
	}

	/* catch sig int so that we can politely close networks on kill */
	signal(SIGINT, DropFromNetwork);
	
	/* set directory that will be used */
	dartsync_dir = tilde_expand(DARTSYNC_DIR);
	if (!dartsync_dir) {
		fprintf(stderr, "Failed to expand %s\n", DARTSYNC_DIR);
		exit(-1);
	}

	/* check if the folder already exists, if it doesn't then make it */
	if (0 != access(dartsync_dir, (F_OK)) ){
		printf("Cannot access %s -- creating directory\n", dartsync_dir);
		perror("reason");
		/* it doesn't exist, so make it */
		if (-1 == mkdir(dartsync_dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)){
		//if (system("mkdir ~/dart_sync") != 0) {
			printf("CLIENT MAIN: failed to create DARTSYNC_DIR (%s)\n",dartsync_dir);
			exit(-1);
		}
	} 

	/* get the current local filesystem */
	if (NULL == (cur_fs = filesystem_new(dartsync_dir))){
		printf("CLIENT MAIN: filesystem_new() failed\n");
		DestroyPeerTable();
		filetable_destroy(ft);
		exit(-1);
	}
	filesystem_print(cur_fs);

	/* send a request to the tracker for the master filesystem. 
	 * this will check against our current filesystem and make requests for updates */
	SendMasterFSRequest(cur_fs);

	/* start the loop process that will run while we are connected to the tracker 
	 * this will handle peer adds and dels and receive updates from the master 
	 * filesystem and monitor the local filesystem */
	int new_client = -1, del_client = -1, peer_id = -1, chunk_id = -1, len = -1;
	FileSystem *master;
	int recv_len;
	while (1){
		sleep(POLL_STATUS_DIFF);

		/* get any new clients first */
		// printf("CLIENT MAIN: checking for new clients\n");
		while (-1 != (new_client = recv_peer_added(cnt))){
			if (-1 == InsertPeer(new_client, CONN_ACTIVE)){
				printf("CLIENT MAIN: InsertPeer() failed\n");
			}
			new_client = -1;
		}

		/* figure out if any clients have dropped from the network */
		// printf("CLIENT MAIN: checking for deleted clients\n");
		while (-1 != (del_client = recv_peer_deleted(cnt))){
			if (-1 == RemovePeer(del_client)){
				printf("CLIENT MAIN: RemovePeer() failed\n");
			}
			del_client = -1;
		}

		/* check to see if any requests for files have been made to you */
		// printf("CLIENT MAIN: checking for chunk requests\n");
		char *filepath;
		while (-1 != receive_chunk_request(cnt, &peer_id, &filepath, &chunk_id)){
			printf("CLIENT MAIN: received chunk request from peer: %d\n", peer_id);

			/* get the chunk that they are requesting */
			ChunkyFile *file = chunkyfile_new_from_path(filepath);

			if (!file){	// need to send a rejection message somehow
				printf("chunkyfile_new_from_path() failed on %s\n", filepath);

				/* send an error response */
				send_chunk_rejection(cnt, peer_id, filepath, chunk_id);
				continue;
			}

			// should chunk be already malloc'd?  what about chunk size?
			char *chunk_text;
			int chunk_len;
			if (GET_ALL_CHUNKS == chunk_id){	// send the entire file
				int num_chunks = chunkyfile_num_chunks(file);

				for (int i = 0; i < num_chunks; i++){
					printf("CLIENT MAIN: sending chunk %d to peer %d\n", i, peer_id);

					chunkyfile_get_chunk(file, i, &chunk_text, &chunk_len);

					send_chunk(cnt, peer_id, filepath, i, chunk_text, chunk_len);
				}
			} else {
				chunkyfile_get_chunk(file, chunk_id, &chunk_text, &chunk_len);

				/* send that chunk to the peer */
				send_chunk(cnt, peer_id, filepath, chunk_id, chunk_text, chunk_len);
			}

			/* destroy that chunky file */
			chunkyfile_destroy(file);

			filepath = NULL;
		}

		/* poll for any received chunks */
		// printf("CLIENT MAIN: checking for chunks received\n");
		char *chunk_data;
		int chunk_id;
		while (-1 != receive_chunk(cnt, &peer_id, &filepath, &chunk_id, 
				&len, &chunk_data)){
			printf("CLIENT MAIN: received chunk %d for file %s\n", chunk_id, filepath);

			/* if len is -1, then we received a rejection response */
			if (-1 == len){	// how should I handle this???
				printf("CLIENT MAIN: receive_chunk got a rejection response\n");
				continue;
			}

			/* open the chunky file */
			ChunkyFile *file = chunkyfile_new_from_path(filepath);

			/* set the correct chunk */
			chunkyfile_set_chunk(file, chunk_id, chunk_data, len);

			// You should write the chunkyfile to a path here :) 
			chunkyfile_write_to_path(file, filepath);

			/* destroy the chunky file */
			chunkyfile_destroy(file);

			free(chunk_data);
			filepath = NULL;
			chunk_data = NULL;
		}

		/* poll for any diffs, if they exist then we need to make updates to 
		 * our filesystem */
		// printf("CLIENT MAIN: checking for diffs\n");
		FileSystem *additions, *deletions;
		int author_id;
		while (-1 != recv_diff(cnt, &additions, &deletions, &author_id)){
			printf("CLIENT MAIN: received a diff from author %d\n", author_id);
			/* set the root paths of the additions and deletions filesystems */
			filesystem_set_root_path(additions, dartsync_dir);
			filesystem_set_root_path(deletions, dartsync_dir);

			/* get rid of the deletions */
			RemoveFileDeletions(deletions);

			/* remove them from the file table */
			filetable_remove_filesystem(ft, deletions);

			/* the originator of this diff has the master, so tell the filetable */
			filetable_add_filesystem(ft, additions, author_id);

			/* get the file additions from peers, and update the filesystem */
			GetFileAdditions(additions, author_id);

			/* get ready for next recv diff or iteration of loop */
			filesystem_destroy(deletions);
			filesystem_destroy(additions);
			additions = NULL;
			deletions = NULL;
		}

		/* poll to see if there are any changes to the master file system 
		 * if there are then we need to get those parts from peers and then
		 * copy master to our local pointer of the filesystem */
		// printf("CLIENT MAIN: checking for updates from master\n");
		while (NULL != (master = recv_master(cnt, &recv_len))){
			printf("CLIENT MAIN: received master from tracker\n");
			/* set the root path of the master filesystem */
			printf("dartsync_dir: %s\n", dartsync_dir);
			fflush(stdout);
			filesystem_set_root_path(master, dartsync_dir);

			filesystem_print(master);

			UpdateLocalFilesystem(master);
		}

		/* check the local filesystem for changes that we need to push
		 * to master */
		// printf("CLIENT MAIN: checking local file system\n");
		CheckLocalFilesystem();
	}

}



