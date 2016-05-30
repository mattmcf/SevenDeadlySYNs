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
FileSystem *prev_fs = NULL;
FileTable *ft;
char * dartsync_dir; 	// global of absolute dartsync_dir path
int myID = 0;

/* ------------------------------- TODO -------------------------------- */

/* add password recognition */

/* ----------------------------- Questions ----------------------------- */

// should chunk be already malloc'd?  what about chunk size?
// how to destroy a chunky file and a chunk?

/* ---------------------- Private Function Headers --------------------- */

/* check if file system is empty, used to check diffs for anything */
int CheckFileSystem(FileSystem *fs);

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

// takes an absolute file path and replaces the path to dart_sync with a tilda
//	original_path: (not claimed) the file path to compress
// 	ret: (not claimed) the compressed filepath
char * tilde_compress(char * original_path){
	if (!original_path){
		return NULL;
	}
	// printf("Tilde compressing string %s\n", original_path);
	
	for (int i = strlen(original_path)-1; i >= 9; i--){
		printf("i = %d\n", i);
		if (original_path[i] == 'c' &&
			original_path[i-1] == 'n' &&
			original_path[i-2] == 'y' &&
			original_path[i-3] == 's' &&
			original_path[i-4] == '_' &&
			original_path[i-5] == 't' &&
			original_path[i-6] == 'r' &&
			original_path[i-7] == 'a' &&
			original_path[i-8] == 'd'
			)
		{
			char buffer[strlen(original_path)-i+10]; // + /dart_sync\0
			memset(buffer, 0 , sizeof(buffer));
			// printf("moving: %s\n", &original_path[i+1]);
			memcpy(buffer, &original_path[i-9], strlen(original_path)-(i-9)*sizeof(char));
			buffer[strlen(original_path)-i+9+1] = '\0';
			// printf("set new original address\n");
			// original_path += (i+1)*sizeof(char);
			// printf("New original path: %s\n", buffer);
			char * compressed_string = (char*)malloc(sizeof(buffer)+sizeof(char));
			sprintf(compressed_string, "~%s", buffer);
			// printf("compressed_string: %s\n", compressed_string);
			return compressed_string;
		}
	}
	return NULL;
}

/* ----------------------- Public Function Bodies ---------------------- */

int SendMasterFSRequest(FileSystem *cur_fs){
	FileSystem *master;

	// if (-1 == send_status(cnt, cur_fs)){
	// 	printf("SendMasterFSRequest: send_status failed\n");
	// 	return -1;
	// }
	printf("Send master FS request\n");
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
	printf("Received master file system\n");

	/* set root of returned file system */
	filesystem_set_root_path(master, dartsync_dir);

	/* todo -- should also receive master file table */
	int master_ft_len = 0;
	while (NULL == (ft = recv_master_ft(cnt, &master_ft_len))){
		printf("SendMasterFTRequest: recv_master_ft return NULL, sleeping\n");
		sleep(1);
	}
	printf("Received master file table\n");

	UpdateLocalFilesystem(master);
	return 1;
}

void UpdateLocalFilesystem(FileSystem *new_fs){
	printf("UpdateLocalFilesystem: received update from master!\n");

	/* received the master file system, iterate over it to look for differences */
	FileSystem *additions;
	FileSystem *deletions;
	filesystem_diff(cur_fs, new_fs, &additions, &deletions);

	if (!deletions){
		printf("UpdateLocalFilesystem: deletions is NULL\n");
	} else if (-1 == CheckFileSystem(deletions)){
		printf("UpdateLocalFilesystem: deletions is empty\n");
	} else {
		/* iterate over additions to see if we need to request files */
		/* delete any files that we need to update, or remove flat out */
		RemoveFileDeletions(deletions);
	}

	if (!additions){
		printf("UpdateLocalFilesystem: additions is NULL\n");
	} else if (-1 == CheckFileSystem(additions)){
		printf("UpdateLocalFilesystem: additions is empty\n");
	} else {
		/* iterate over additions to see if we need to request files */
		printf("UpdateLocalFilesystem: ready to iterate over additions!\n");
		FileSystemIterator* add_iterator = filesystemiterator_new(additions);

		if (!add_iterator){
			printf("UpdateLocalFilesystem: failed to make add iterator\n");
		}

		char *path;
		int len;
		time_t mod_time;
		while (NULL != (path = filesystemiterator_next(add_iterator, &len, &mod_time))){
			printf("UpdateLocalFilesystem: found addition at: %s\n", path);

			/* if addition is just a directory -> make that and then go onto files */
			if (len == -1) {

				if (-1 == mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)){
				// char new_dir[1024];
				// snprintf(new_dir, sizeof(new_dir), "mkdir %s", path);
				// if (system(new_dir) != 0) {
					printf("UpdateLocalFilesystem: failed to create directory \'%s\'\n", path);
					perror("Failed because");
				}
				continue;
			}

			/* re make that deleted file */
			ChunkyFile *file = chunkyfile_new_for_writing_to_path(path, len, mod_time);
			if (!file){
				printf("UpdateLocalFilesystem: chunkyfile_new_empty() failed()\n");
				continue;
			}
			chunkyfile_write(file);

			// ADD CHUNKYFILE TO HASH TABLE!!!!!!!!!!
			filetable_set_chunkyfile(ft, path, file);

			/* figure out how many chunks we need to request */
			int num_chunks = chunkyfile_num_chunks(file);
			if (-1 == num_chunks){
				printf("UpdateLocalFilesystem: chunkyfile_num_chunks() failed\n");
			}

			/* for each chunk, find a peer who has it, and request it */
			for (int i = 0; i < num_chunks; i++){ //???
				/* get the peers who have the chunk that we want */
				printf("UpdateLocalFilesystem: looking for peers with %s's chunk %d\n", path, i);
				Queue *peers = filetable_get_peers_who_have_file_chunk(ft, path, i);
				if (!peers){
					printf("UpdateLocalFilesystem: filetable_get_peers_who_have_file_chunk() failed\n");
					continue;
				}

				/* randomly select one peer to get the chunk from */
				int list_id = (rand()*100) % queue_length(peers);
				int peer_id = (int)(long)queue_get(peers, list_id);
				if (peer_id == myID){
					peer_id = (int)(long)queue_get(peers, list_id);
				}

				/* make the request to get that chunk */
				printf("UpdateLocalFilesystem: requesting chunk %d of %s from %d\n",
						i, path, peer_id);
				send_chunk_request(cnt, peer_id, path, i);
			}

			/* get ready for next iteration */
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

void CheckLocalFilesystem()
{
	FileSystem *new_fs = filesystem_new(dartsync_dir);
	FileSystem *adds = NULL, *dels = NULL;

	if (!new_fs)
	{
		printf("CheckLocalFilesystem: filesystem_new() failed\n");
		return;
	}

	if (prev_fs == NULL || !filesystem_equals(prev_fs, new_fs))
	{
		printf("CheckLocalFilesystem: Local filesystem is changing. Waiting to settle before reporting diffs.\n")
		if (prev_fs)
		{
			filesystem_destroy(prev_fs);
		}
		prev_fs = new_fs;
		return;
	}

	filesystem_diff(cur_fs, new_fs, &adds, &dels);

	if (1 == CheckFileSystem(adds)) 
	{
		printf("CheckLocalFilesystem: Additions Seen ----\n");
		filesystem_print(adds);
	}

	if (1 == CheckFileSystem(dels)) 
	{
		printf("CheckLocalFilesystem: Deletions Seen -----\n");
		filesystem_print(dels);
	}

	/* if there are either additions or deletions, then we need to let the 
	 * master know */
	if (!adds && !dels)
	{
		printf("CheckLocalFilesystem: diff failed\n");
		filesystem_destroy(new_fs);
	} 
	else if ((1 == CheckFileSystem(adds)) || (1 == CheckFileSystem(dels)))
	{
		printf("CheckLocalFilesystem: about to send diffs to the tracker\n");
		/* send the difs to the tracker */
		if (-1 == send_updated_files(cnt, adds, dels))
		{
			printf("CheckLocalFilesystem: send_updated_files() failed\n");
		}

		// update the file table
		filetable_remove_filesystem(ft, dels);
		filetable_add_filesystem(ft, adds, myID);

		/* update the pointer to our *current* filesystem */
		filesystem_destroy(cur_fs);
		filesystem_destroy(adds);
		filesystem_destroy(dels);
		cur_fs = NULL;
		cur_fs = new_fs; 
	} 
	else 
	{
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
	filetable_destroy(ft);

	free(dartsync_dir);

	exit(0);
}

int RemoveFileDeletions(FileSystem *deletions){
	printf("RemoveFileDeletions: ready to iterate over deletions!\n");
	FileSystemIterator* del_iterator = filesystemiterator_new(deletions);
	char *path;
	int len;
	time_t mod_time;
	
	if (!del_iterator){
		printf("RemoveFileDeletions: failed to make del iterator\n");
		return -1;
	}

	/* iterate over the fileystem, delete any files that are in it */
	while (NULL != (path = filesystemiterator_next(del_iterator, &len, &mod_time))){
		printf("RemoveFileDeletions: found deletion at: %s\n", path);
		if (-1 == remove(path)){
			printf("RemoveFileDeletions: remove() failed on path\n");
		}

		path = NULL;
	}

	printf("RemoveFileDeletions: done deleting\n");
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
	time_t mod_time;
	while (NULL != (path = filesystemiterator_next(add_iterator, &len, &mod_time))){
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
		ChunkyFile* file = chunkyfile_new_for_writing_to_path(path, len, mod_time);
		if (!file){
			printf("GetFileAdditions: failed to open new chunkyfile\n");
			continue;
		}
		//chunkyfile_write(file);
		
		/* write that file to the path */
		printf("chunky file write to path: %s\n", path);

		// ADD CHUNKYFILE TO HASH TABLE!!!!!
		filetable_set_chunkyfile(ft, path, file);
		
		/* request all chunks */
		path = tilde_compress(path);
		printf("Send chunk request for file %s\n", path);
		send_chunk_request(cnt, author_id, path, GET_ALL_CHUNKS);

		path = NULL;
	}

	filesystemiterator_destroy(add_iterator);
	return 1;
}

/* ----------------------- Private Function Bodies --------------------- */

int CheckFileSystem(FileSystem *fs){
	char *path;
	int len;
	time_t mod_time;
	FileSystemIterator *iterator = filesystemiterator_new(fs);

	if (NULL != (path = filesystemiterator_next(iterator, &len, &mod_time))){
		printf("CheckFileSystem: FileSystem is nonempty\n");
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

	/* initialize the psuedorandom number generator */
	srand(time(NULL));

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
	int peer_id = -1, chunk_id = -1, len = -1;
	FileSystem *master;
	int recv_len;
	while (1){
		sleep(POLL_STATUS_DIFF);

		/* get any new clients first */
		// printf("CLIENT MAIN: checking for new clients\n");
		while (-1 != (peer_id = recv_peer_added(cnt))){
			printf("CLIENT MAIN: new peer %d\n", peer_id);
			peer_id = -1;
		}

		/* figure out if any clients have dropped from the network */
		// printf("CLIENT MAIN: checking for deleted clients\n");
		while (-1 != (peer_id = recv_peer_deleted(cnt))){
			printf("CLIENT MAIN: deleting peer %d\n", peer_id);
			/* delete from file table */
			filetable_remove_peer(ft, peer_id);
			peer_id = -1;
		}

		/* check to see if any requests for files have been made to you */
		// printf("CLIENT MAIN: checking for chunk requests\n");
		char *filepath;
		while (-1 != receive_chunk_request(cnt, &peer_id, &filepath, &chunk_id)){
			printf("CLIENT MAIN: received chunk request from peer: %d\n", peer_id);
			char *expanded_path = tilde_expand(filepath);
			/* get the chunk that they are requesting */
			ChunkyFile *file = chunkyfile_new_for_reading_from_path(expanded_path);

			if (!file){	
				printf("chunkyfile_new_for_reading_from_path() failed on %s\n", expanded_path);

				/* send an error response */
				send_chunk_rejection(cnt, peer_id, filepath, chunk_id);
				peer_id = -1;
				continue;
			}

			char *chunk_text;
			int chunk_len;
			if (GET_ALL_CHUNKS == chunk_id){	// send the entire file
				int num_chunks = chunkyfile_num_chunks(file);

				for (int i = 0; i < num_chunks; i++){
					printf("CLIENT MAIN: sending %s chunk %d to peer %d\n", expanded_path, i, peer_id);

					chunkyfile_get_chunk(file, i, &chunk_text, &chunk_len);

					send_chunk(cnt, peer_id, filepath, i, chunk_text, chunk_len);
				}
			} else {

				printf("CLIENT MAIN: sending chunk (%s, %d) to peer %d", expanded_path, chunk_id, peer_id);

				chunkyfile_get_chunk(file, chunk_id, &chunk_text, &chunk_len);

				/* send that chunk to the peer */
				send_chunk(cnt, peer_id, filepath, chunk_id, chunk_text, chunk_len);
			}

			/* destroy that chunky file */
			chunkyfile_destroy(file);

			peer_id = -1;
			filepath = NULL;
		}

		/* poll for any received chunks */
		// printf("CLIENT MAIN: checking for chunks received\n");
		char *chunk_data;
		int chunk_id;
		while (-1 != receive_chunk(cnt, &peer_id, &filepath, &chunk_id, 
				&len, &chunk_data)){
			char *expanded_path = tilde_expand(filepath);
			printf("CLIENT MAIN: received chunk %d for file %s\n", chunk_id, expanded_path);

			/* if len is -1, then we received a rejection response */
			if (-1 == len){	// how should I handle this???
				printf("CLIENT MAIN: receive_chunk got a rejection response\n");
				continue;
			}

			// GET THE CHUNKYFILE FROM THE HASH TABLE!!!!!
			ChunkyFile* file = filetable_get_chunkyfile(ft, expanded_path);
			if (!file){
				printf("CLIENT MAIN: failed to get chunkfile from ft on receive_chunk\n");
				continue;
			}

			/* set the correct chunk */
			chunkyfile_set_chunk(file, chunk_id, chunk_data, len);

			// TODO -- SEND THE FILE ACQ TO THE TRACKER
			send_chunk_got(cnt, filepath, chunk_id);

			if (chunkyfile_all_chunks_written(file)){
	
				chunkyfile_write(file);
				/* destroy the chunky file */
				chunkyfile_destroy(file);
				filetable_set_chunkyfile(ft, expanded_path, NULL);
			}

			/* destroy the old filesystem struct and recreate it to reflect changes */
			filesystem_destroy(cur_fs);
			cur_fs = filesystem_new(dartsync_dir);

			free(chunk_data);
			peer_id = -1;
			filepath = NULL;
			chunk_data = NULL;
		}

		/* check for chunk aquisition updates and add them to our file table */
		while (-1 != receive_chunk_got(cnt, &peer_id, &filepath, &chunk_id)){
			char *expanded_path = tilde_expand(filepath);
			printf("CLIENT MAIN: received chunk acq update from %d on file %s chunk %d\n", 
					peer_id, expanded_path, chunk_id);

			filetable_set_that_peer_has_file_chunk(ft, expanded_path, peer_id, chunk_id);
			peer_id = -1;
		}

		/* poll for any diffs, if they exist then we need to make updates to 
		 * our filesystem */
		// printf("CLIENT MAIN: checking for diffs\n");
		FileSystem *additions, *deletions;
		while (-1 != recv_diff(cnt, &additions, &deletions, &peer_id)){
			printf("CLIENT MAIN: received a diff from author %d\n", peer_id);
			/* set the root paths of the additions and deletions filesystems */
			filesystem_set_root_path(additions, dartsync_dir);
			filesystem_set_root_path(deletions, dartsync_dir);

			/* get rid of the deletions */
			RemoveFileDeletions(deletions);

			/* update current fs to reflect the deletions */
			filesystem_minus_equals(cur_fs, deletions);

			/* remove them from the file table */
			filetable_remove_filesystem(ft, deletions);

			/* update current file with additions (time stamped) */
			filesystem_plus_equals(cur_fs, additions);

			/* the originator of this diff has the master, so tell the filetable */
			filetable_add_filesystem(ft, additions, peer_id);

			/* get the file additions from peers, and update the filesystem */
			GetFileAdditions(additions, peer_id);

			/* update the current filesystem pointer */
			filesystem_destroy(cur_fs);
			cur_fs = filesystem_new(dartsync_dir);

			/* get ready for next recv diff or iteration of loop */
			filesystem_destroy(deletions);
			filesystem_destroy(additions);
			additions = NULL;
			deletions = NULL;
			peer_id = -1;
		}

		/* poll to see if there are any changes to the master file system 
		 * if there are then we need to get those parts from peers and then
		 * copy master to our local pointer of the filesystem */
		// printf("CLIENT MAIN: checking for updates from master\n");
		while (NULL != (master = recv_master(cnt, &recv_len))){

			printf("CLIENT MAIN: received master from tracker\n");
			/* set the root path of the master filesystem */
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



