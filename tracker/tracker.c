/*
* Dartmouth College CS60 Final Project
* DartSync
*
*/

#include <arpa/inet.h>          // inet_ntoa
#include <signal.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <sys/sendfile.h>
// #include <sys/uio.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <wordexp.h> // for shell expansion of ~

#include "tracker.h"
#include "../common/constant.h"
#include "network_tracker.h"
#include "../utility/FileSystem/FileSystem.h"
#include "../utility/FileTable/FileTable.h"

char * dartsync_dir;
FileSystem* fs;
FileTable* filetable;
peer_table* peerTable;
int peerTableSize = 16;
static volatile int keepRunning = 1;

void intHandler(int dummy) {
    keepRunning = 0;
}

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

int main() {
	signal(SIGINT, intHandler);
	int peerID = -1;

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
			printf("TRACKER MAIN: failed to create DARTSYNC_DIR\n");
			exit(-1);
		}
	} 

	// create file system
	fs = filesystem_new(dartsync_dir);
	filesystem_print(fs);

	filetable = filetable_new();
	filetable_print(filetable);

	pthread_t browser_thread;
	pthread_create(&browser_thread, NULL, webBrowser, (void*)0);

	// create peer table
	peerTable = createPeerTable();

	// start network thread
	TNT *network = StartTrackerNetwork();

	if (!network) {
		fprintf(stderr,"Something went wrong while spinning up the network thread\n");
		return 1;
	}

	// main thread
	while(keepRunning){

		// If there is a new peer in the Queue
			// get peerID and add peer to peer table
			// broadcast to all other peers that there is a new peer
		// printf("checking for new client\n");
		peerID = -1;
		while ((peerID = receive_new_client(network))>0){
			if (addPeerToTable(peerID)<0){	// maybe print peer table after this
				printf("\tFailed to add peer %d to peer table.\n", peerID);
			}
			// send_transaction_update(network, &fs, peerID);
			printf("\tSend peer added\n");
			newPeerBroadcast(peerID, network);
			peerID = -1;
		}

		// If a peer requests master
			// send master
		peerID = -1;
		// printf("checking for master request\n");
		while ((peerID = receive_master_request(network))>0){
			printf("Send master\n");
			if(send_master(network, peerID, fs)<0){
				printf("\tFailed to send master to peer %d\n", peerID);
			}
			if(send_master_filetable(network, peerID, filetable)<0) {
				printf("\tFailed to send master file table to peer %d\n", peerID);
			}

			peerID = -1;
		}
		// printf("\tcheck master request peer: %d\n", peerID);

		// if there is a peer disconnect
			// get peerID and remove peer from peer table
			// broadcast to all otehr peers that peer left
		// printf("checking for lost client\n");
		peerID = -1;
		while ((peerID = receive_lost_client(network))>0){
			if(removePeerFromTable(peerID)<0){
				printf("\tFailed to remove peer %d from table.\n", peerID);
			}
			filetable_remove_peer(filetable, peerID);
			printf("\tRemoved peer %d from table.\n", peerID);
			printf("\tSend peer removed\n");
			lostPeerBroadcast(peerID, network);
			peerID = -1;
		}

		// See if any clients have a chunk acquisition update to deseminate
		char * file_got = (char*)malloc(200*sizeof(char)); // i dont think there will be a longer filepath than that
		int chunk_got, peer_got_id;
		while(receive_client_got(network, file_got, &chunk_got, &peer_got_id) == 1) {
			printf("\tClient %d received chunk %d of %s\n", peer_got_id, chunk_got, file_got);
			// let everyone know that a peer got a chunk
			clientGotBroadcast(file_got, chunk_got, network, peer_got_id);
			// update file table
			filetable_print(filetable);
			filetable_set_that_peer_has_file_chunk(filetable, file_got, peer_got_id, chunk_got);
		}
		free(file_got);

		// if there is a file update
			// take the diff
			// apply diff to local fs
			// broadcast diff to all peers
		// printf("Checking peer updates\n");
		FileSystem *additions;
		FileSystem *deletions;
		int *clientID = (int*)malloc(sizeof(int));
		while(receive_client_update(network, clientID, &additions, &deletions)>0){
			printf("File Update Received\n");
			updateNetwork(network, clientID, additions, deletions);
			printf("\tFinished updating file system\n");
		}
		free(clientID);
		sleep(5);
		printf("Restarting loop.\n");
	}

	if (closeTracker()<0) {
	 	printf("Failed to close everything. Quitting anyway.\n");
	} 
	EndTrackerNetwork(network);
	free(dartsync_dir);
	printf("buh-bye\n");

	return 1;
}

// ****************************************************************
//						Handling Peer Table	
// ****************************************************************

// ceates a new peer table. allocates the memory
peer_table* createPeerTable(){
	peer_table *newPeerTable = (peer_table*)malloc(sizeof(peer_table));
	newPeerTable->numberOfEntries = 0;
	newPeerTable->peerIDs = (int *)malloc(peerTableSize*sizeof(int));
	for(int i = 0; i < peerTableSize; i++){
		newPeerTable->peerIDs[i] = -1;
	}
	return newPeerTable;
}

// destroys peer table and frees memory
int destroyPeerTable(peer_table* deleteTable){
	printPeerTable();
	printf("free peerIDs\n");
	free(deleteTable->peerIDs);
	printf("free table\n");
	free(deleteTable);
	printf("Done destroying peer table\n");
	return 1;
}

// Takes a new peer and adds them to a table based on unique ID
int addPeerToTable(int peerID) {
	peerTable->numberOfEntries++;
	// find free slot and add peer id
	int i;
	for(i = 0; i < peerTableSize; i++){
		// if peerID is already in table, function 
		if (peerTable->peerIDs[i] == peerID){
			printPeerTable();
			return -1;
		}
		// if an empty slot is found
		if (peerTable->peerIDs[i] == -1){
			peerTable->peerIDs[i] = peerID;
			printPeerTable();
			return 1;
		}
	}
	// all slots full. double the number of slots
	peerTableSize = peerTableSize * 2;
	int* temp = (int*)malloc(peerTableSize*sizeof(int));
	int* delete;
	delete = peerTable->peerIDs;
	memmove(temp, delete, sizeof(int)*(peerTableSize/2));
	peerTable->peerIDs = temp;
	free(delete);
	// save peer as the next index (old peer size + 1)
	// i++;
	for (int n = peerTableSize/2; n < peerTableSize; n++){
		peerTable->peerIDs[n] = -1;
	}
	peerTable->peerIDs[i] = peerID;
	printPeerTable();
	return 1;
}

// Removes peer after they leave the network
int removePeerFromTable(int peerID){
	int i = 0;
	// either get to end of table or find peer
	while (i < peerTableSize && peerTable->peerIDs[i] != peerID){
		i++;
	}
	printf("At position: %d\n", i);
	// if found peer
	if (peerTable->peerIDs[i] == peerID){
		peerTable->peerIDs[i] = -1;
		peerTable->numberOfEntries--;
		// TODO: LET NETWORK KNOW TO BROADCAST TO EVERYONE
		printPeerTable();
		return 1;
	}
	// could not find peer to remove
	printf("Peer %d not in table.\n", peerID);
	printPeerTable();
	return -1;
}

// print the peer table
int printPeerTable(){
	printf("\n####################\n     Peer Table     \n####################\n\n");
	printf("Contains %d peers:\n", peerTable->numberOfEntries);
	for (int i = 0; i < peerTableSize; i++){
		if (peerTable->peerIDs[i] == -1){
			printf("Entry %d: empty\n", i);
		} else{
			printf("Entry %d: %d\n", i, peerTable->peerIDs[i]);
		}
	}
	return 1;
}


// ****************************************************************
//						Sending Messages	
// ****************************************************************

//close everything, free memory
int closeTracker(){
	printf("Close tracker\n");
	destroyPeerTable(peerTable);
	filesystem_print(fs);
	filesystem_destroy(fs);
	printf("All freed, closing tracker\n");
	return 1;
}

// broadcast to all peers that there is a new peer
int newPeerBroadcast(int newPeerID, TNT *network){
	for (int i = 0; i < peerTableSize; i++){
		if(peerTable->peerIDs[i] != -1 && peerTable->peerIDs[i] != newPeerID){
			if(send_peer_added(network, peerTable->peerIDs[i], newPeerID)<0){ //HOW DO WE INDICATE WHAT PEER SHOULD RECEIVE
				printf("Failed to send new peer update to peer %d\n", peerTable->peerIDs[i]);
			}
		}
	}
	return 1;
}

// broadcast to all peers that there is a peer removed
int lostPeerBroadcast(int lostPeerID, TNT *network){
	for (int i = 0; i < peerTableSize; i++){
		if(peerTable->peerIDs[i] != -1 && peerTable->peerIDs[i] != lostPeerID){
			if(send_peer_removed(network, peerTable->peerIDs[i], lostPeerID)<0){ //HOW DO WE INDICATE WHAT PEER SHOULD RECEIVE
				printf("Failed to send lost peer update to peer %d\n", peerTable->peerIDs[i]);
			}
		}
	}
	return 1;
}

// broadcast to all peers that there is a file update
int filesystemUpdateBroadcast(FileSystem * additions, FileSystem * deletions, TNT *network, int originator){
	for (int i = 0; i < peerTableSize; i++){
		if(peerTable->peerIDs[i] != -1 && peerTable->peerIDs[i] != originator){
			if(send_FS_update(network, peerTable->peerIDs[i], originator, additions, deletions)<0){
				printf("Failed to send transaction update to peer %d\n", peerTable->peerIDs[i]);
			}
		}
	}
	return 1;
}

// broadcast to all peers that a client GOT
int clientGotBroadcast(char * file_got, int chunk_got, TNT *network, int peer_got_id){
	printf("Distributing chunk acquisition update (owner %d, file: %s, chunk: %d)\n",peer_got_id,file_got,chunk_got);
	for (int i = 0; i < peerTableSize; i++) {
		// send to all peers (except author of acq)
		if(peerTable->peerIDs[i] != -1 && peerTable->peerIDs[i] != peer_got_id){
			if (send_got_chunk_update(network, peerTable->peerIDs[i], peer_got_id, file_got, chunk_got) != 1) {
				printf("\tFailed to send File Acq (%s, %d) to client %d\n", file_got, chunk_got, peerTable->peerIDs[i]);
			}
		}				
	}

	return 1;
}

// identifies files that the peer needs and sends table of files and clients
int sendUpdates(int peerID){
	printf("sendUpdates.\n");
	return 1;
}



// ****************************************************************
//						Update mode Fucntionality	
// ****************************************************************
int updateNetwork(TNT* network, int updatePusher, FileSystem *additions, FileSystem *deletions){
	// update file system

	filesystem_set_root_path(additions, filesystem_get_root_path(fs));
	filesystem_set_root_path(deletions, filesystem_get_root_path(fs));
	printf("PRINTING ADDITIONS:\n");
	filesystem_print(additions);
	printf("PRINTING DELETIONS:\n");
	filesystem_print(deletions);
	filesystem_minus_equals(fs, deletions);
	filesystem_plus_equals(fs, additions);
	printf("Updating file table\n");
	filetable_remove_filesystem(filetable, deletions);
	filetable_add_filesystem(filetable, additions, updatePusher);
	printf("File table updated\n");
	filetable_print(filetable);
	// broadcast out update to all peers
	filesystemUpdateBroadcast(additions, deletions, network, updatePusher);
	printf("NEW FILE SYSTEM\n");
	filesystem_print(fs);
	filesystem_destroy(additions);
	filesystem_destroy(deletions);
	// while file table update is not complete:
	// use this: TKR_2_CLT_FILE_ACQ and CLT_2_TKR_CLIENT_GOT
		// if acquisition update
			// update file table
			// broadcast out update
	return 1;
}

int isNetworkUpdated(){
	// Use this function to check if the file being updated hasbeen fully updated
	return 1;
}





// ****************************************************************
//						Browser Interface Controls	
// ****************************************************************
 
mime_map meme_types [] = {
    {".css", "text/css"},
    {".gif", "image/gif"},
    {".htm", "text/html"},
    {".html", "text/html"},
    {".jpeg", "image/jpeg"},
    {".jpg", "image/jpeg"},
    {".ico", "image/x-icon"},
    {".js", "application/javascript"},
    {".pdf", "application/pdf"},
    {".mp4", "video/mp4"},
    {".png", "image/png"},
    {".svg", "image/svg+xml"},
    {".xml", "text/xml"},
    {NULL, NULL},
};

char *default_mime_type = "text/plain";
int browserArray[10];
char ipArray[10][39];

DIR *fdopendir(int fd);
// int openat(int dirfd, const char *pathname, int flags);

int numberOfLines(char *fileName){
    FILE *fp = fopen(fileName, "r");
    int lines = 0;
    char ch;
    while(!feof(fp)){
        ch = fgetc(fp);
        if(ch == '\n'){
            lines++;
        }
    }
    return lines;
}

void addNewBrowser(char *browserName){
    // FILE *output = fopen("temp.txt", "w+");
    // FILE *source = fopen("browser_history.txt", "r");
    // char buffer[100];
    // size_t bytesRead;

    // memset(buffer, 0, sizeof(buffer));

    // fprintf(output, "%s\n", browserName);

    // int iter = 0;
    // while((fgets(buffer, sizeof(buffer), source) != NULL) && iter < 9){
    //     fprintf(output, buffer);
    //     iter ++;
    //     memset(buffer, 0, sizeof(buffer));
    // }
    // fclose(source);
    // fclose(output);


    // FILE *output2 = fopen("temp.txt", "r");
    // FILE *source2 = fopen("browser_history.txt", "w+");

    //  while(!feof(output2)){
    //     bytesRead = fread(&buffer, 1, sizeof(buffer), output2);
    //     fwrite(&buffer, 1, bytesRead, source2);
    // }
    // fclose(output2);
    // fclose(source2);

}

void addNewIP(char *ip){
    // FILE *output = fopen("temp2.txt", "w+");
    // FILE *source = fopen("ip_addresses.txt", "r");
    // char buffer[100];
    // size_t bytesRead;

    // memset(buffer, 0, sizeof(buffer));

    // // fprintf(output, "%s\n", ip);

    // int iter = 0;
    // while((fgets(buffer, sizeof(buffer), source) != NULL) && iter < 9){
    //     // fprintf(output, buffer);
    //     iter ++;
    // }
    // // fclose(source);
    // // fclose(output);

    // // FILE *output2 = fopen("temp2.txt", "r");
    // // FILE *source2 = fopen("ip_addresses.txt", "w+");

    //  while(!feof(output2)){
    //     bytesRead = fread(&buffer, 1, sizeof(buffer), output2);
    //     fwrite(&buffer, 1, bytesRead, source2);
    // }
    // fclose(output2);
    // fclose(source2);
}

// set up an empty read buffer and associates an open file descriptor with that buffer
void rio_readinitb(rio_t *rp, int fd){
    rp->rio_fd = fd;
    rp->rio_cnt = 0;
    rp->rio_bufptr = rp->rio_buf;
}

// utility function for writing user buffer into a file descriptor
ssize_t written(int fd, void *usrbuf, size_t n){
    size_t nleft = n;
    ssize_t nwritten;
    char *bufp = usrbuf;

    while (nleft > 0){
        if ((nwritten = write(fd, bufp, nleft)) <= 0){
            if (errno == EINTR)  // interrupted by sig handler return
                nwritten = 0;    // and call write() again
            else
                return -1;       // errorno set by write()
        }
        nleft -= nwritten;
        bufp += nwritten;
    }
    return n;
}


/*
 *    This is a wrapper for the Unix read() function that
 *    transfers min(n, rio_cnt) bytes from an internal buffer to a user
 *    buffer, where n is the number of bytes requested by the user and
 *    rio_cnt is the number of unread bytes in the internal buffer. On
 *    entry, rio_read() refills the internal buffer via a call to
 *    read() if the internal buffer is empty.
 */
 static ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n){
    int cnt;
    while (rp->rio_cnt <= 0){  // refill if buf is empty

        rp->rio_cnt = read(rp->rio_fd, rp->rio_buf,
           sizeof(rp->rio_buf));
        if (rp->rio_cnt < 0){
            if (errno != EINTR) // interrupted by sig handler return
                return -1;
        }
        else if (rp->rio_cnt == 0)  // EOF
            return 0;
        else
            rp->rio_bufptr = rp->rio_buf; // reset buffer ptr
    }
    
    // copy min(n, rp->rio_cnt) bytes from internal buf to user buf
    cnt = n;
    if (rp->rio_cnt < n)
        cnt = rp->rio_cnt;
    memcpy(usrbuf, rp->rio_bufptr, cnt);
    rp->rio_bufptr += cnt;
    rp->rio_cnt -= cnt;
    return cnt;
}

// robustly read a text line (buffered)
ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen){
    int n, rc;
    char c, *bufp = usrbuf;
    
    for (n = 1; n < maxlen; n++){
        if ((rc = rio_read(rp, &c, 1)) == 1){
            *bufp++ = c;
            if (c == '\n')
                break;
        } else if (rc == 0){
            if (n == 1)
                return 0; // EOF, no data read
            else
                break;    // EOF, some data was read
        } else
            return -1;    // error
        }
        *bufp = 0;
        return n;
    }

// utility function to get the format size
    void format_size(char* buf, struct stat *stat){
        if(S_ISDIR(stat->st_mode)){
            sprintf(buf, "%s", "[DIR]");
        } else {
            off_t size = stat->st_size;
            if(size < 1024){
                sprintf(buf, "%lu", size);
            } else if (size < 1024 * 1024){
                sprintf(buf, "%.1fK", (double)size / 1024);
            } else if (size < 1024 * 1024 * 1024){
                sprintf(buf, "%.1fM", (double)size / 1024 / 1024);
            } else {
                sprintf(buf, "%.1fG", (double)size / 1024 / 1024 / 1024);
            }
        }
    }

// pre-process files in the "home" directory and send the list to the client
// DONE
void handle_directory_request(int out_fd, int dir_fd, char *filename){
    char buf[MAXLINE];

    // send response headers to client e.g., "HTTP/1.1 200 OK\r\n"
    sprintf(buf, "HTTP/1.1 200 OK\r\n%s%s%s%s%s",
        "Content-Type: text/html\r\n\r\n",
        "<html><head><style>",
        "body{font-family: monospace; font-size: 13px;}",
        "td {padding: 1.5px 6px;}",
        "</style></head><body><table><tbody>\n");
    written(out_fd, buf, strlen(buf));

    // send recent browser data to the client
    
    sprintf(buf, "</tbody></table><table><tbody><tr>Peer Table, %d Peers:</tr>\n", peerTable->numberOfEntries);
    written(out_fd, buf, strlen(buf));

     //    printf("\n####################\n     Peer Table     \n####################\n\n");
	// printf("Contains %d peers:\n", peerTable->numberOfEntries);
	for (int i = 0; i < peerTableSize; i++){
	 	if (peerTable->peerIDs[i] == -1){
	 		sprintf(buf, "<tr><td>Entry %d: empty</td></tr>\n",i);
        	written(out_fd, buf, strlen(buf));
		} else{
			sprintf(buf, "<tr><td>Entry %d: %d</td></tr>\n",i, peerTable->peerIDs[i]);
        	written(out_fd, buf, strlen(buf));
		}
	}
	sprintf(buf, "</tbody></table></body></html>");
    written(out_fd, buf, strlen(buf));
}

// utility function to get the MIME (Multipurpose Internet Mail Extensions) type
    static const char* get_mime_type(char *filename){
        char *dot = strrchr(filename, '.');
    if(dot){ // strrchar Locate last occurrence of character in string
        mime_map *map = meme_types;
        while(map->extension){
            if(strcmp(map->extension, dot) == 0){
                return map->mime_type;
            }
            map++;
        }
    }
    return default_mime_type;
}

// DONE
// open a listening socket descriptor using the specified port number.
int open_listenfd(int port){
    int listenfd, optval=1;
    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));

    // create a socket descriptor
    printf("\tCreate socket\n");
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0){
        perror("Failed to create socket.\n");
        return -1;
    }

    // eliminate "Address already in use" error from bind.
    if ( setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *) &optval , sizeof(int)) < 0){
        return -1;
    }

    // 6 is TCP's protocol number
    // enable this, much faster : 4000 req/s -> 17000 req/s
    // if ( setsockopt(listenfd, 6, TCP_CORK, (const void *)&optval , sizeof(int)) < 0){
    //     return -1;
    // }

    // Listenfd will be an endpoint for all requests to port
    // on any IP address for this host
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)port);

    printf("\tBind\n");
    // bind socket
    if (bind(listenfd, (SA *)&serveraddr, sizeof(serveraddr)) < 0){
        //perror("Failed to bind socket. Try another port number.\n");
        return -1;
    }
    printf("\tListen\n");
    // make it a listening socket ready to accept connection requests
    if (listen(listenfd, LISTENQ) < 0){
        //perror("Failed to listen.\n");
        return -1;
    }
    printf("Listening on port %d.\n", port);

    return listenfd;
}

// parse request to get url
// DONE
void parse_request(int fd, http_request *req){
    rio_t rio;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE], browser[MAXLINE];

    // set to default
    req->offset = 0;
    req->end = 0; 

    // Rio (Robust I/O) Buffered Input Functions
    rio_readinitb(&rio, fd);
    rio_readlineb(&rio, buf, MAXLINE);
    // get the file request here
    sscanf(buf, "%s %s %s", method, uri, version);
    memset(browser, 0, sizeof(browser));
    // read all
    // while(buf[0] != '\n' && buf[1] != '\n' && browserNotFound) { /* \n || \r\n */
    //     rio_readlineb(&rio, buf, MAXLINE);
    //     // Get browser data
    //     if(buf[0] == 'U' && buf[1] == 's' && buf[2] == 'e' && buf[3] == 'r' && buf[4] == '-' && buf[5] == 'A' && buf[6] == 'g'){
    //             // set browser based on what substring user-agent contains
    //         if (strstr(buf, "Chrome/") != NULL){
    //           strcpy(browser,"Chrome");
    //         }else if ((strstr(buf, "Firefox/") != NULL)){
    //             strcpy(browser,"Firefox");
    //         }else if ((strstr(buf, "Opera/") != NULL)){
    //             strcpy(browser,"Opera");
    //         }
    //         else if ((strstr(buf, "Safari/") != NULL)){
    //             strcpy(browser,"Safari");
    //         }else strcpy(browser,"Other");
    //         browserNotFound = 0;
    //         // addNewBrowser(browser);
    //     }
    // }
    // scan the file request and decode it
    char* file = uri;
    if(uri[0] == '/'){
        file = uri + 1;
        int length = strlen(file);
        if (length == 0){
            file = ".";
        } else {
            for (int i = 0; i < length; ++ i) {
                if (file[i] == '?') {
                    file[i] = '\0';
                    break;
                }
            }
        }
    }
    // decode url
    int max = MAXLINE;
    char *p = file;
    char* destination = req->filename;
    char code[3] = { 0 };
    while(*p && --max) {
        if(*p == '%') {
            memcpy(code, ++p, 2);
            *destination++ = (char)strtoul(code, NULL, 16);
            p += 2;
        } else {
            *destination++ = *p++;
        }
    }
    *destination = '\0';
}

// echo client error e.g. 404
void client_error(int fd, int status, char *msg, char *longmsg){
    char buf[MAXLINE];
    sprintf(buf, "HTTP/1.1 %d %s\r\n", status, msg);
    sprintf(buf + strlen(buf),
            "Content-length: %lu\r\n\r\n", strlen(longmsg));
    sprintf(buf + strlen(buf), "%s", longmsg);
    written(fd, buf, strlen(buf));
}

// DONE
// serve static content
void serve_static(int out_fd, int in_fd, http_request *req, size_t total_size){
    char buf[256];
    if (req->offset > 0){
        // send response headers to client e.g., "HTTP/1.1 200 OK\r\n"
        sprintf(buf, "HTTP/1.1 206 Partial\r\n");
        sprintf(buf + strlen(buf), "Content-Range: bytes %lu-%lu/%lu\r\n", req->offset, req->end, total_size);
    } else {
        sprintf(buf, "HTTP/1.1 200 OK\r\nAccept-Ranges: bytes\r\n");
    }
    sprintf(buf + strlen(buf), "Cache-Control: no-cache\r\n");
    sprintf(buf + strlen(buf), "Content-length: %lu\r\n",req->end - req->offset);
    sprintf(buf + strlen(buf), "Content-type: %s\r\n\r\n",get_mime_type(req->filename));
    written(out_fd, buf, strlen(buf));

    off_t offset = req->offset; 
    while(offset < req->end){
        // send response body to client
        // if(sendfile(out_fd, in_fd, &offset, req->end - req->offset) <= 0) {
        //     break;
        // }
        close(out_fd);
        break;
    }    
}

// DONE
// handle one HTTP request/response transaction
void process(int fd, struct sockaddr_in *clientaddr){
	printf("Process\n");
    http_request req;
    parse_request(fd, &req);
    printf("Finished parsing\n");

    struct stat sbuf;
    int status = 200; //server status init as 200
    // addNewIP(inet_ntoa(clientaddr->sin_addr));
    int ffd = open(req.filename, O_RDONLY, 0);
    if(ffd <= 0){
        // detect 404 error and print error log
        status = 404;
        printf("Not Found\n");
        char *message = "File not found";
        client_error(fd, status, "Not found", message);
    } else {
    	printf("else\n");
        // get descriptor status
        fstat(ffd, &sbuf);
        if(S_ISREG(sbuf.st_mode)){
            // server serves static content
            if (req.end == 0){
                req.end = sbuf.st_size;
            }
            if (req.offset > 0){
                status = 206;
            }
            printf("Serve static\n");
            serve_static(fd, ffd, &req, sbuf.st_size);

        } else if(S_ISDIR(sbuf.st_mode)){
            // server handle directory request
            status = 200;
            printf("Directory request\n");
            handle_directory_request(fd, ffd, req.filename);
        } else {
            // detect 400 error and print error log
            status = 400;
            char *message = "Unknow Error";
            printf("Error\n");
            client_error(fd, status, "Error", message);
        }
        printf("close\n");
        close(ffd);
    }
}

// http://stackoverflow.com/questions/29248585/c-checking-command-line-argument-is-integer-or-not
int isNumber(char number[]){
    int i = 0;
    //checking for negative numbers
    if (number[0] == '-')
        i = 1;
    for (; number[i] != 0; i++)
    {
        //if (number[i] > '9' || number[i] < '0')
        if (!isdigit(number[i]))
            return 0;
    }
    return 1;
}
// check user input. if there are errors, return -1 and give guidance for future attempts
int checkargs(int argc, char* argv[]){
    struct stat directoryCheck;
    if (argc != 3){
        printf("ERROR: Invalid number of arguments. \n");
        printf("You need 2 arguments.\n");
        printf( "Use the form  file_browser [PORT NUMBER] [DIRECTORY] \n");
        return -1;
    }

    if (!isNumber(argv[1])){
        printf("ERROR: Expected a port number. Did not receive a number. \n");
        printf( "Use the form  file_browser [PORT NUMBER] [DIRECTORY] \n");
        return -1;
    }

    if( stat(argv[2], &directoryCheck) != 0){
        printf("ERROR: The directory does not exist.\n");
        printf("Please check your file path and ensure the directory exists.\n");
        printf( "Use the form  file_browser [PORT NUMBER] [DIRECTORY] \n");
        printf( "For the directory, use '.' if you want the current directory. If you want a subdirectory, use './subDir'.\n");
        return -1;
    }
    return 1;
}

// main function:
// get the user input for the file directory and port number
void * webBrowser(){
	printf("In file browser\n");
    struct sockaddr_in clientaddr;
    int default_port = 10467, listenfd, sockfd;
    
    // // user input checking
    // if (checkargs(argc, argv) == -1){
    //     return 0;
    // }

    // get the name of the current working directory
    // char *path = getcwd(buf, 256);
    // printf("current path: %s\n", path);
    socklen_t clientlen = sizeof clientaddr;
    // create files to track usage or erase previous content

    memset(ipArray, 0, sizeof(ipArray));
    memset(browserArray, 0, sizeof(browserArray));

    // set port
    // default_port = atoi(argv[1]);

    // set file path
    // path = argv[2];
    // if(chdir(argv[2]) != 0) {
    //     perror(argv[2]);
    //     exit(1);
    // }

    // FILE *ip = fopen("ip_addresses.txt", "w+");
    // FILE *browsers = fopen("browser_history.txt", "w+");
    // fclose(ip);
    // fclose(browsers);
    // if (argv[2][0] == '.') 
    //     memmove(argv[2], argv[2]+1, strlen(argv[2]));
    // strcat(path, argv[2]);
    // printf("current path: %s\n", path);
    printf("\tListening\n");
    listenfd = open_listenfd(default_port);
    if (listenfd < 0){
        perror("Failed to listen. Try another Port.");
        pthread_exit(NULL);
    }

    // ignore SIGPIPE signal, so if browser cancels the request, it
    // won't kill the whole process.
    signal(SIGPIPE, SIG_IGN);

    // permit an incoming connection attempt on a socket.
   while(1){
   		printf("\tWaiting for accept\n");
        sockfd = accept(listenfd, (SA *)&clientaddr, &clientlen);
        printf("Accepted connection.\n");
        int pid = fork();
        if (pid == 0){
            close(listenfd);
            process(sockfd, &clientaddr);
            exit(0);
        }else if (pid < 0) {   //  parent
            perror("Error on fork");
            pthread_exit(NULL);
        } else {
            close(sockfd);
        }
    }
    close(listenfd);
    pthread_exit(NULL);
}


// for testing purposes
// int main(){
// 	peerTable = createPeerTable();
// 	addPeerToTable(1);
// 	addPeerToTable(2);
// 	addPeerToTable(3);
// 	addPeerToTable(4);
// 	addPeerToTable(5);
// 	addPeerToTable(6);
// 	addPeerToTable(7);
// 	addPeerToTable(8);
// 	addPeerToTable(9);
// 	addPeerToTable(10);
// 	addPeerToTable(11);
// 	addPeerToTable(12);
// 	addPeerToTable(13);
// 	addPeerToTable(14);
// 	removePeerFromTable(6);
// 	removePeerFromTable(7);
// 	removePeerFromTable(8);
// 	addPeerToTable(15);
// 	addPeerToTable(16);
// 	addPeerToTable(17);
// 	addPeerToTable(18);
// 	addPeerToTable(19);
// 	addPeerToTable(20);
// 	removePeerFromTable(1);
// 	removePeerFromTable(2);
// 	removePeerFromTable(3);
// 	removePeerFromTable(4);
// 	removePeerFromTable(5);
// 	removePeerFromTable(9);
// 	removePeerFromTable(10);
// 	removePeerFromTable(11);
// 	removePeerFromTable(12);
// 	removePeerFromTable(13);
// 	removePeerFromTable(14);
// 	removePeerFromTable(15);
// 	removePeerFromTable(16);
// 	removePeerFromTable(17);
// 	removePeerFromTable(18);
// 	removePeerFromTable(19);
// 	removePeerFromTable(20);
// 	destroyPeerTable(peerTable);
// 	printf("\nDone with test\n");
// 	return 1;
// }



