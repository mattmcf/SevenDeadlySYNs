/*
 * network_client.c
 *
 * contains functionality for client-side network thread
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <strings.h>
#include <arpa/inet.h>
#include <signal.h>
#include <netdb.h>
#include <assert.h>
#include <sys/utsname.h>
#include <pthread.h>
#include <unistd.h>
#include <netdb.h>

#include "network_client.h"
#include "../common/constant.h"
#include "../common/peer_table.h"
#include "../common/packets.h"
#include "../utility/AsyncQueue/AsyncQueue.h" 
#include "../utility/FileSystem/FileSystem.h" 

#define INIT_PEER_SIZE 10

/* --- 	queue definitions --- */
#define TKR_2_ME_TRANSACTION_UPDATE 0
#define TKR_2_ME_FILE_ACQ 1
#define TKR_2_ME_PEER_ADDED 2
#define TKR_2_ME_PEER_DELETED 3
#define TKR_2_ME_RECEIVE_MASTER_JFS 4

#define CLT_2_ME_REQUEST_CHUNK 0
#define CLT_2_ME_RECEIVE_CHUNK 1

#define ME_2_TKR_CUR_STATUS 0
#define ME_2_TKR_ACQ_UPDATE 1
#define ME_2_TKR_QUIT 2
#define ME_2_TKR_GET_MASTER 3

#define ME_2_CLT_REQ_CHUNK 0
#define ME_2_CLT_SEND_CHUNK 1
#define ME_2_CLT_SEND_ERROR 2

typedef struct _CNT {

	/*
	 * *** INCOMING QUEUES ***
	 * 
	 * -- tracker 2 client --
	 * tkr_queues_to_client[0] -> transaction update queue (JFS of updated files)
	 * tkr_queues_to_client[1] -> file acquisition status queue
	 * tkr_queues_to_client[2] -> peer added queue
	 * tkr_queues_to_client[3] -> peer deleted queue
	 * tkr_queues_to_client[4] -> master JFS
	 * 
	 * -- client 2 client --
	 * clt_queues_to_client[0] -> receive request for chunk
	 * clt_queues_to_client[1] -> receive chunk from peer (can be error)
	 * 
	 */
	AsyncQueue ** tkr_queues_to_client; 	// to logic from tracker
	AsyncQueue ** clt_queues_to_client; 	// to logic from client

	/*
	 * *** OUTGOING QUEUES ***
	 * 
	 * -- client to tracker --
	 * tkr_queues_from_client[0] -> send current status / (join network)
	 * tkr_queues_from_client[1] -> send acquisition update to tracker (successful & unsucessful)
	 * tkr_queues_from_client[2] -> send quit message
	 * tkr_queues_from_client[3] -> send request for master
	 *
	 * -- client 2 client --
	 * clt_queues_from_client[0] -> send request for chunk
	 * clt_queues_from_client[1] -> send chunk
	 * clt_queues_from_client[2] -> send an error to client
	 *
	 */
	AsyncQueue ** tkr_queues_from_client; 	// from logic to tracker
	AsyncQueue ** clt_queues_from_client; 	// from logic to client

	/*
	 * network monitoring information
	 */
	pthread_t thread_id;
	char * ip_addr;
	int ip_len;
	int tracker_fd;
	int peer_listening_fd;

	peer_table_t * peer_table;

} _CNT_t;

// network thread start point
void * clt_network_start(void * arg);

CNT * StartClientNetwork(char * ip_addr, int ip_len) {

	if (!ip_addr) {
		fprintf(stderr,"StartClientNetwork: bad ip address\n");
		return NULL;
	}

	_CNT_t * client_thread = (_CNT_t *)calloc(1,sizeof(_CNT_t));

	/* -- incoming client to tracker -- */
	client_thread->tkr_queues_to_client = (AsyncQueue **)calloc(5, sizeof(AsyncQueue *));
	client_thread->tkr_queues_to_client[TKR_2_ME_TRANSACTION_UPDATE] = asyncqueue_new();
	client_thread->tkr_queues_to_client[TKR_2_ME_FILE_ACQ] = asyncqueue_new();
	client_thread->tkr_queues_to_client[TKR_2_ME_PEER_ADDED] = asyncqueue_new();
	client_thread->tkr_queues_to_client[TKR_2_ME_PEER_DELETED] = asyncqueue_new();
	client_thread->tkr_queues_to_client[TKR_2_ME_RECEIVE_MASTER_JFS] = asyncqueue_new();

	/* -- incoming client to client -- */
	client_thread->clt_queues_to_client = (AsyncQueue **)calloc(2,sizeof(AsyncQueue *));
	client_thread->clt_queues_to_client[CLT_2_ME_REQUEST_CHUNK] = asyncqueue_new();
	client_thread->clt_queues_to_client[CLT_2_ME_RECEIVE_CHUNK] = asyncqueue_new();

	/* -- outgoing client to tracker -- */
	client_thread->tkr_queues_from_client = (AsyncQueue **)calloc(4,sizeof(AsyncQueue *));
	client_thread->tkr_queues_from_client[ME_2_TKR_CUR_STATUS] = asyncqueue_new();
	client_thread->tkr_queues_from_client[ME_2_TKR_ACQ_UPDATE] = asyncqueue_new();
	client_thread->tkr_queues_from_client[ME_2_TKR_QUIT] = asyncqueue_new();
	client_thread->tkr_queues_from_client[ME_2_TKR_GET_MASTER] = asyncqueue_new();

	/* -- outgoing client to client -- */
	client_thread->clt_queues_from_client = (AsyncQueue **)calloc(3,sizeof(AsyncQueue *));
	client_thread->clt_queues_from_client[ME_2_CLT_REQ_CHUNK] = asyncqueue_new();
	client_thread->clt_queues_from_client[ME_2_CLT_SEND_CHUNK] = asyncqueue_new();
	client_thread->clt_queues_from_client[ME_2_CLT_SEND_ERROR] = asyncqueue_new();

	/* -- save tracker server arguments -- */
	client_thread->ip_addr = malloc(ip_len);
	memcpy(client_thread->ip_addr, ip_addr, ip_len);
	client_thread->ip_len = ip_len;

	client_thread->peer_table = init_peer_table(INIT_PEER_SIZE);
	if (!client_thread->peer_table) {
		fprintf(stderr,"client network thread failed to create peer table\n");
		return NULL;
	}

	/* -- spin off network thread -- */
	pthread_create(&client_thread->thread_id, NULL, clt_network_start, client_thread);

	return (CNT *)client_thread;
}

// nicely ends network
void EndClientNetwork(CNT * thread_block) {
	printf("NETWORK -- cleaning up\n");

	_CNT_t * cnt = (_CNT_t *)thread_block;
	if (!cnt)
		return;

	asyncqueue_destroy(cnt->tkr_queues_to_client[TKR_2_ME_TRANSACTION_UPDATE]);
	asyncqueue_destroy(cnt->tkr_queues_to_client[TKR_2_ME_FILE_ACQ]);
	asyncqueue_destroy(cnt->tkr_queues_to_client[TKR_2_ME_PEER_ADDED]);
	asyncqueue_destroy(cnt->tkr_queues_to_client[TKR_2_ME_PEER_DELETED]);
	asyncqueue_destroy(cnt->tkr_queues_to_client[TKR_2_ME_RECEIVE_MASTER_JFS]);
	free(cnt->tkr_queues_to_client);

	asyncqueue_destroy(cnt->clt_queues_to_client[CLT_2_ME_REQUEST_CHUNK]);
	asyncqueue_destroy(cnt->clt_queues_to_client[CLT_2_ME_RECEIVE_CHUNK]);
	free(cnt->clt_queues_to_client);

	asyncqueue_destroy(cnt->tkr_queues_from_client[ME_2_TKR_CUR_STATUS]);
	asyncqueue_destroy(cnt->tkr_queues_from_client[ME_2_TKR_ACQ_UPDATE]);
	asyncqueue_destroy(cnt->tkr_queues_from_client[ME_2_TKR_QUIT]);
	asyncqueue_destroy(cnt->tkr_queues_from_client[ME_2_TKR_GET_MASTER]);
	free(cnt->tkr_queues_from_client);

	asyncqueue_destroy(cnt->clt_queues_from_client[ME_2_CLT_REQ_CHUNK]);
	asyncqueue_destroy(cnt->clt_queues_from_client[ME_2_CLT_SEND_CHUNK]);
	asyncqueue_destroy(cnt->clt_queues_from_client[ME_2_CLT_SEND_ERROR]);
	free(cnt->clt_queues_from_client);

	free(cnt->ip_addr);

	// close all peer connections
	for (int i = 0; i < cnt->peer_table->size; i++) {
		if (cnt->peer_table->peer_list[i] && cnt->peer_table->peer_list[i]->socketfd > 0) 
			close(cnt->peer_table->peer_list[i]->socketfd);
	}

	destroy_table(cnt->peer_table);

	close(cnt->tracker_fd);
	close(cnt->peer_listening_fd);

	free(cnt);

	printf("NETWORK -- ending client network thread after cleaning up\n");
	
	return;
}

/* ###################### *
 * 
 * Things that the client should do to interact with network thread
 *
 * ###################### */

/* ----- receiving ----- */

// receive transaction update : TKR_2_ME_TRANSACTION_UPDATE
int recv_diff(CNT * thread, FileSystem ** additions, FileSystem ** deletions) {

	return -1;
}

// receive acq status update : TKR_2_ME_FILE_ACQ

// receive peer added message : TKR_2_ME_PEER_ADDED
//	thread_block : (not claimed) thread block
// 	ret : (static) id of added client (always > 1) or -1 if no new clients
int recv_peer_added(CNT * thread_block) {
	_CNT_t * cnt = (_CNT_t *)thread_block;
	int new_id = (int)(long)asyncqueue_pop(cnt->tkr_queues_to_client[TKR_2_ME_PEER_ADDED]);
	return (new_id > 0) ? new_id : -1;
}

// receive peer deleted message : TKR_2_ME_PEER_DELETED
// 	thread_block : (not claimed) thread block
// 	ret : (static) if of a deleted client (> 1) or -1 if no new clients
int recv_peer_deleted(CNT * thread_block) {
	_CNT_t * cnt = (_CNT_t *)thread_block;
	int deleted_id = (int)(long)asyncqueue_pop(cnt->tkr_queues_to_client[TKR_2_ME_PEER_DELETED]);
	return (deleted_id > 0) ? deleted_id : -1;
}

// receive master JFS from tracker : TKR_2_ME_RECEIVE_MASTER_JFS
// 	CNT : (not claimed) thread block
// 	length_deserialized : (not claimed) will be filled in with length deserialized if there was an update
//	ret : (not claimed) master file system -- needs to be claimed 
//				null if no update
FileSystem * recv_master(CNT * thread, int * length_deserialized) {
	_CNT_t * cnt = (_CNT_t *)thread;
	client_data_t * queue_item = asyncqueue_pop(cnt->tkr_queues_to_client[TKR_2_ME_RECEIVE_MASTER_JFS]);

	FileSystem * fs = NULL;
	if (queue_item) {
		printf("LOGIC -- Receiving master off of queue\n");
		//fs = filesystem_deserialize(queue_item->data, length_deserialized);
		fs = queue_item->data;
		
		//free(queue_item->data);
		free(queue_item);
	}

 	return fs;
}

// receive request for chunk : CLT_2_ME_REQUEST_CHUNK

// receive chunk from peer : CLT_2_ME_RECEIVE_CHUNK


/* ----- sending ----- */

// send current status : ME_2_TKR_CUR_STATUS
// 	thread : (not claimed) thread block
// 	fs : (not claimed) filesystem to send
// 	ret : (static) 1 on success, -1 on failure
int send_status(CNT * thread, FileSystem * fs) {
	if (!thread || !fs)
		return -1;
	
	_CNT_t * cnt = (_CNT_t *)thread;

	client_data_t * queue_item = malloc(sizeof(client_data_t));
	filesystem_serialize(fs, (char **)&queue_item->data, &queue_item->data_len);
	asyncqueue_push(cnt->tkr_queues_from_client[ME_2_TKR_CUR_STATUS], (void *)queue_item);
	return 1;
}

// send file acquistion update : ME_2_TKR_ACQ_UPDATE

// send quit message to tracker : ME_2_TKR_QUIT

// send request for master JFS to tracker : ME_2_TKR_GET_MASTER
// returns 1 on success, -1 on failure
int send_request_for_master(CNT * cnt) {
	_CNT_t * thread_block = (_CNT_t *) cnt;
	asyncqueue_push(thread_block->tkr_queues_from_client[ME_2_TKR_GET_MASTER], (void*)(long)1);
	return 1;
}

// send request for chunk to peer : ME_2_CLT_REQ_CHUNK

// send chunk to client : ME_2_CLT_SEND_CHUNK

// send chunk request error response : ME_2_CLT_SEND_ERROR


/* ###################### *
 * 
 * Things that the network can do to interact with the client
 *
 * ###################### */

// puts transaction update on 
int notify_transaction_update(_CNT_t * cnt, tracker_pkt_t * pkt);

// pushes received master onto queue
//	returns 1 on success, -1 on failure
int notify_master_received(_CNT_t * cnt, client_data_t * queue_item) {
	if (!cnt || !queue_item)
		return -1;

	asyncqueue_push(cnt->tkr_queues_to_client[TKR_2_ME_RECEIVE_MASTER_JFS], (void *)queue_item);
	return 1;
}

int notify_file_acq_update() {

	return -1;
}

int notify_peer_added(_CNT_t * cnt, int added_client_id) {
	if (!cnt)
		return -1;

	asyncqueue_push(cnt->tkr_queues_to_client[TKR_2_ME_PEER_ADDED], (void *)(long)added_client_id);
	return 1;
}

int notify_peer_removed(_CNT_t * cnt, int removed_client_id) {
	if (!cnt)
		return -1;

	asyncqueue_push(cnt->tkr_queues_to_client[TKR_2_ME_PEER_DELETED], (void *)(long)(removed_client_id));
	return 1;
}

/* ------------------------ FUNCTION DECLARATIONS ------------------------ */

/* --- Network Side Packet Functions --- */

// returns listening socket fd
int connect_to_tracker(char * ip_addr, int ip_len);

int handle_tracker_msg(_CNT_t * cnt);
int handle_peer_msg(int sockfd, _CNT_t * cnt);

int send_tracker_message(_CNT_t * cnt, client_data_t * data_item, client_to_tracker_t type);
int send_peer_message(_CNT_t * cnt, int peer_id);

void send_heartbeat(_CNT_t * cnt);

/* --- Network Side Queue functions --- */
void poll_queues(_CNT_t * cnt);
void check_cur_status_q(_CNT_t * cnt);
void check_file_acq_q(_CNT_t * cnt);
void check_quit_q(_CNT_t * cnt);
void check_master_req_q(_CNT_t * cnt);
void check_req_chunk_q(_CNT_t * cnt);
void check_send_chunk_q(_CNT_t * cnt);
void check_req_error_q(_CNT_t * cnt);

/* ------------------------ NETWORK THREAD ------------------------ */

void * clt_network_start(void * arg) {

	_CNT_t * cnt = (_CNT_t *)arg;
	if (!cnt) {
		fprintf(stderr,"cannot start client network thread because argument is null\n");
		return (void *)-1;
	}

	while ( (cnt->tracker_fd = connect_to_tracker(cnt->ip_addr, cnt->ip_len)) < 0) {
		printf("Failed to connect with tracker...\n");
		sleep(5);
	}

	// set up timer
	struct timeval timeout;
	timeout.tv_sec = NETWORK_WAIT;
	timeout.tv_usec = 0;

	// select fd sets
	fd_set read_fd_set, active_fd_set;
	FD_ZERO(&active_fd_set);

	// add listening socket to fd set
	FD_SET(cnt->tracker_fd, &active_fd_set);

	/* need to open peer listening socket */
	int peer_listening_fd = -1;

	time_t last_heartbeat = 0, current_time;
	int connected = 1;
	while (connected) {

		read_fd_set = active_fd_set;
		if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, &timeout) < 0) {
			fprintf(stderr, "network client failed to select amongst inputs\n");
			connected = 0;
			continue;
		}

		//printf("\nclient network polling connections...\n");
		for (int i = 0; i < FD_SETSIZE; i++) {
			if (FD_ISSET(i, &read_fd_set)) {

				// from tracker
				if (i == cnt->tracker_fd) {

					printf("client network received message from tracker (socket %d)\n", i);
					if (handle_tracker_msg(cnt) != 1) {
						fprintf(stderr, "failed to handle tracker message. Ending.\n");
						connected = 0;
						continue;
					}

				// new peer connection
				} else if (i == peer_listening_fd) {

					printf("client network received new connection from peer (socket %d)\n",i);
					// need to accept peer connection

					// new_sockfd = accept(peer_listening_fd, (struct sockaddr *)&clientaddr, &addrlen);
					// if (new_sockfd < 0) {
					// 	fprintf(stderr,"network tracker accept_connections thread failed to accept new connection\n");
					// 	continue;
					// }

				// else peer message
				} else {
					printf("client network received message from peer on socket %d\n", i);
					if (handle_peer_msg(i, cnt) != 1) {
						fprintf(stderr, "failed to handle client message. Ending.\n");
						connected = 0;
						continue;
					}
				}
			}

		} 	// end of socket scan

		/* send heart beat if necessary */
		current_time = time(NULL);
		if (current_time - last_heartbeat > (DIASTOLE)) {
			send_tracker_message(cnt, NULL, HEARTBEAT);
			last_heartbeat = current_time;
		}

		/* poll queues for messages from client logic */
		poll_queues(cnt);

	}

	return (void *)1;
}

/* ------------------------ HANDLE PACKETS ON NETWORK ------------------------ */

// connects to the tracker
//	returns connected socket if successful
// 	else returns -1
int connect_to_tracker(char * ip_addr, int ip_len) {
	if (!ip_addr)
		return -1;

	int sockfd;
	if ((sockfd = socket(AF_INET6, SOCK_STREAM, 6)) < 0) {
		perror("client_network thread failed to create tracker socket");
		return -1;
	}

	// fill out server struct
	struct sockaddr_in6 servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	memcpy(&servaddr.sin6_addr, ip_addr, ip_len);
	servaddr.sin6_family = AF_INET6;
	servaddr.sin6_port = htons(TRACKER_LISTENING_PORT); 

	// connect
	if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
		perror("client_network thread failed to connect to tracker");
		return -1;
	}

	char ip_str[INET6_ADDRSTRLEN] = "";
	if (!inet_ntop(AF_INET6, &servaddr.sin6_addr, ip_str, INET6_ADDRSTRLEN)) {
		perror("inet_ntop failed");
		return -1;
	}

	printf("NETWORK connected to tracker at %s on port %d (socket %d)\n", ip_str,TRACKER_LISTENING_PORT,sockfd);
	return sockfd;
}

int handle_tracker_msg(_CNT_t * cnt) {
	if (!cnt)
		return -1;

	tracker_pkt_t pkt;
	if (recv(cnt->tracker_fd, &pkt, sizeof(tracker_pkt_t), 0) != sizeof(tracker_pkt_t)){
		fprintf(stderr,"client network failed to get message from tracker\n");
		return -1;
	}

	char * buf = NULL;
	if (pkt.data_len > 0) {
		buf = calloc(1,pkt.data_len);
		if (recv(cnt->tracker_fd, buf, pkt.data_len, 0) != pkt.data_len) {
			fprintf(stderr, "client network failed to get data from tracker\n");
			return -1;
		}
	}

	switch (pkt.type) {
		case TRANSACTION_UPDATE:
			printf("NETWORK -- received transaction update from tracker\n");
			// process transaction update
			
			break;

		case MASTER_STATUS:
			printf("NETWORK -- received master JFS from tracker\n");
			client_data_t * master_update = malloc(sizeof(client_data_t));	
			master_update->data = (void*)filesystem_deserialize(buf, &master_update->data_len);
			if (!master_update->data || master_update->data_len < 0) {
				fprintf(stderr, "failed to unpack master JFS\n");
				break;
			}

			notify_master_received(cnt, master_update);
			break;

		case PEER_TABLE:
			printf("NETWORK -- received peer table from tracker\n");

			peer_table_t * additions, * deletions, * new_table;
			new_table = deserialize_peer_table(buf, pkt.data_len); 	// buf is claimed
			buf = NULL; 

			if (!new_table) {
				fprintf(stderr, "client network failed to deserialize tracker's peer table\n");
				break;
			}

			diff_tables(cnt->peer_table, new_table, &additions, &deletions);

			// notify logic about additions
			for (int i = 0; i < additions->size; i++) {
				if (additions->peer_list[i] != NULL) {
					notify_peer_added(cnt, additions->peer_list[i]->id);
				}
			}

			// notify logic about deletions
			for (int i = 0; i < deletions->size; i++) {
				if (deletions->peer_list[i] != NULL) {
					notify_peer_removed(cnt, deletions->peer_list[i]->id);
				}
			}

			destroy_table(additions);
			destroy_table(deletions);
			destroy_table(cnt->peer_table);

			// replace current peer table with received peer table
			cnt->peer_table = new_table;
			printf("\n new peer table\n");
			print_table(cnt->peer_table);
			break;

		case FILE_ACQ_UPDATE:
		case PEER_ADDED:
		case PEER_DELETED:
		default:
			printf("CLIENT NETWORK received unhandled packet of type %d\n", pkt.type);
			break;
	}

	if (buf != NULL) {
		free(buf);
	}
		
	return 1;
}

int handle_peer_msg(int sockfd, _CNT_t * cnt) {

	return -1;
}

int send_tracker_message(_CNT_t * cnt, client_data_t * data_item, client_to_tracker_t type) {
	if (!cnt)
		return -1;

	client_pkt_t pkt;

	switch (type) {
		case HEARTBEAT:
			printf("NETWORK -- sending tracker heartbeat\n");
			pkt.type = HEARTBEAT;
			pkt.data_len = 0;
			if (send(cnt->tracker_fd, &pkt, sizeof(client_pkt_t), 0) != sizeof(client_pkt_t)) {
				perror("client network failed to send heartbeat");
				return -1;
			}
			break;

		default:
			printf("client network cannot send unknown packet type %d to tracker\n", type);
			break;
	}

	return 1;
}

int send_peer_message(_CNT_t * cnt, int peer_id) {

	return -1;
}

/* ------------------------ HANDLE QUEUES TO NETWORK FROM LOGIC ------------------------ */

void poll_queues(_CNT_t * cnt) {
	//printf("\nclient network is polling queues\n");
	check_cur_status_q(cnt);
	check_file_acq_q(cnt);
	check_quit_q(cnt);
	check_master_req_q(cnt);

	check_req_chunk_q(cnt);
	check_send_chunk_q(cnt);
	check_req_error_q(cnt);
}

void check_cur_status_q(_CNT_t * cnt) {
	AsyncQueue * q = cnt->tkr_queues_from_client[ME_2_TKR_CUR_STATUS];

	client_data_t * queue_item = asyncqueue_pop(q);
	if (queue_item) {

		printf("NETWORK -- sending current status JFS to tracker\n");
		client_pkt_t pkt;
		pkt.type = CLIENT_STATE;
		pkt.data_len = queue_item->data_len;
		send(cnt->tracker_fd, &pkt, sizeof(client_pkt_t), 0);
		send(cnt->tracker_fd, queue_item->data, queue_item->data_len, 0);

		free(queue_item->data);
		free(queue_item);
	}

	return;
}

void check_file_acq_q(_CNT_t * cnt) {
	AsyncQueue * q = cnt->tkr_queues_from_client[ME_2_TKR_ACQ_UPDATE];
	return;
}

void check_quit_q(_CNT_t * cnt) {
	AsyncQueue * q = cnt->tkr_queues_from_client[ME_2_TKR_QUIT];
	return;
}

void check_master_req_q(_CNT_t * cnt) {
	AsyncQueue * q = cnt->tkr_queues_from_client[ME_2_TKR_GET_MASTER];
	if (1 == (int)(long)asyncqueue_pop(q)) {

		printf("NETWORK -- sending request for master JFS\n");

		client_pkt_t pkt;
		pkt.type = REQUEST_MASTER;
		pkt.data_len = 0;
		send(cnt->tracker_fd, &pkt, sizeof(client_pkt_t), 0);

	}
	return;
}

void check_req_chunk_q(_CNT_t * cnt) {
	AsyncQueue * q = cnt->clt_queues_from_client[ME_2_CLT_REQ_CHUNK];

	return;
}

void check_send_chunk_q(_CNT_t * cnt) {
	AsyncQueue * q = cnt->clt_queues_from_client[ME_2_CLT_SEND_CHUNK];

	return;
}

void check_req_error_q(_CNT_t * cnt) {
	AsyncQueue * q = cnt->clt_queues_from_client[ME_2_CLT_SEND_ERROR];

	return;
}
