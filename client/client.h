/* 	
 * client.h - 	header file that defines structs and function headers that
 * 				the client will use to monitor its own file system, push  
 * 				changes to a queue that the network will monitor, and monitor
 *				a queue that the network uses to push important changes to us
 *
 */

/* ------------------------- System Libraries -------------------------- */

/* -------------------------- Local Libraries -------------------------- */
#include "network_client.h"

/* ----------------------------- Constants ----------------------------- */
#define REQUEST_JOIN		0
#define REQUEST_CLOSE		1
#define REQUEST_PUSH		2
#define REQUEST_CHUNK		3

/* ----------------------- Structure Definitions ----------------------- */

typedef struct retworkrequestheader{
	int type;

} net_req_hdr_t;

typedef struct networkrequest{
	struct retworkrequestheader header;
	//char data[MAX_SIZE];
} req_t;

typedef struct peer{
	int peer_id;
	int status;
	struct peer *next;
} peer_t;

typedef struct peer_table{
	peer_t *head;
	peer_t *tail;
} peer_table_t;

/* -------------------------- Function Headers ------------------------- */

/* after joining or rejoining a network, we need to compare our copy of the 
 * filesystem with theirs, so we send a request for the master copy to diff
 * with our filesystem.  If there are additions or deletions then we need to
 * request those chunks from peers and update our fs */
int SendMasterFSRequest();

void UpdateLocalFilesystem(FileSystem *cur_fs, FileSystem *new_fs);

/* monitor filesystem */
void CheckLocalFilesystem(FileSystem *cur_fs);

/* drop from network */
void DropFromNetwork();



