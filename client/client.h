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

typedef struct peer{
	int peer_id;
	bool active;
	peer *next;
} peer_t;

typedef struct peer_table{
	peer_t *head;
	peer_t *tail;
} peer_table_t;

/* -------------------------- Function Headers ------------------------- */

/* monitor filesystem */
int MonitorFilesystem();

/* request chunk */
int RequestUpdate();

/* update client table */
int UpdateClientTable();

/* drop from network */
int DropFromNetwork();
