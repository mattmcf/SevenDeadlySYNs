/* 	
 * client.h - 	header file that defines structs and function headers that
 * 				the client will use to monitor its own file system, push  
 * 				changes to a queue that the network will monitor, and monitor
 *				a queue that the network uses to push important changes to us
 *
 */

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
	char data[MAX_SIZE];
} req_t;

/* -------------------------- Function Headers ------------------------- */

/* Start the network thread to connect to the tracker and initialize 
 * queues used to communicate with the network thread
 * 	net_thread_id	:	(not claimed) the network thread identifier 
 *	peer_t_net_q	:	(not claimed) the queue used to pass requests to
 *						the network thread
 */
int connectToNetwork();

/* close connections with tracker and any active client connections,
 * free structures 
 * 	net_thread_id	:	(claimed) the network thread identifier 
 *	peer_t_net_q	:	(claimed) the queue used to pass requests to
 *						the network thread
 */
int dropFromNetwork();

/* monitor filesystem */
int monitorFilesystem();

/* request chunk */
int requestUpdate();

/* update client table */
int updateClientTable();
