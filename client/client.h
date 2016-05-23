/* 	
 * client.h - 	header file that defines structs and function headers that
 * 				the client will use to monitor its own file system, push  
 * 				changes to a queue that the network will monitor, and monitor
 *				a queue that the network uses to push important changes to us
 *
 */

/* ------------------------- System Libraries -------------------------- */

/* -------------------------- Local Libraries -------------------------- */


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

/* monitor filesystem */
int MonitorFilesystem();

/* request chunk */
//int SendChunkRequest(CNT *cnt, int peer_id, char *buf, int len);

/* update client table */
int UpdateClientTable();

/* drop from network */
void DropFromNetwork();

/* ----- requests ----- */

/* send a chunk request from the client app to a peer client 
 * 	cnt - the current state of the network
 *	peer_id - the peer to request the chunk from 
 * 	buf - the chunk that we are requesting
 		TODO - figure out how to combine this with chunky file and
 		how to expand on it to allow for transerring an entire file
 	len - the expected length (DO WE NEED THIS)
 */
int SendChunkRequest(CNT *cnt, int peer_id, char *buf, int len);

/* poll the network to see if we have received a chunk request
 * from a peer client 
 * 	cnt - the current state of the network
 *	peer_id - the peer that made the request
 * 	buf - the chunk that was requested
 		TODO - figure out how to combine this with chunky file and
 		how to expand on it to allow for transerring an entire file
 	len - the expected length (DO WE NEED THIS)
 */
int ReceiveChuckRequest(CNT *cnt, int peer_id, char *buf, int len);

/* client calls this to send a chunk update to another client 
 * 	cnt - current state of the network 
 * 	peer_id - the peer that we are sending it to
 *	buf	- the chunk that we are sending
 * 	len - the length of the chunk that we are sending 
 */
int SendChunk(CNT *cnt, int peer_id, char *buf, int len);

/* client calls this to receive a chunk update from the network 
 * 	cnt - the current state of the network
 * 	(not claimed) ret - the chunk that we received
 */
char* ReceiveChunk(CNT *cnt);




