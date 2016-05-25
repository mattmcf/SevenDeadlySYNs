/*
* Dartmouth College CS60 Final Project
* DartSync
*
*/

#ifndef _TRACKER_H
#define _TRACKER_H


#include "network_tracker.h"
#include "../utility/FileSystem/FileSystem.h"
#include "../utility/ChunkyFile/ChunkyFile.h"

// Peer table structure
typedef struct peer_table {
  int numberOfEntries;		        //number of peers in network
  int *peerIDs;						//peerID number
} peer_table;


void intHandler(int dummy);

// ceates a new peer table. allocates the memory
peer_table* createPeerTable();

// destroys peer table and frees memory
int destroyPeerTable(peer_table* deleteTable);

// Takes a new peer and adds them to a table based on unique ID
int addPeerToTable(int peerID);

// Removes peer after they leave the network
int removePeerFromTable(int peerID);

// print the peer table
int printPeerTable();

//close everything, free memory
int closeTracker();

// broadcast to all peers that there is a new peer
int newPeerBroadcast(int newPeerID, TNT *network);

// broadcast to all peers that there is a peer removed
int lostPeerBroadcast(int lostPeerID, TNT *network);

// broadcast to all peers that there is a file update
int filesystemUpdateBroadcast(FileSystem * additions, FileSystem * deletions, TNT *network);

// peer successfully retrieved master. Update file table
// int clientFileRetrieveSuccess(int peerID, int fileID);
/*
update file system and peer table to reflect that the peer now has new file
*/


// identifies files that the peer needs and sends table of files and clients
int sendUpdates(int peerID);
/*
compare peer file system to saved system
find difference
send difference
*/
// broadcast to all peers that there is a new peer
int newPeerBroadcast(int newPeerID, TNT *network);

// ****************************************************************
//						Web Browser
// ****************************************************************

#define LISTENQ  1024  // second argument to listen()
#define MAXLINE 1024   // max length of a line
#define RIO_BUFSIZE 1024

struct stat directoryCheck;

 typedef struct {
    int rio_fd;                 // descriptor for this buf
    int rio_cnt;                // unread byte in this buf
    char *rio_bufptr;           // next unread byte in this buf
    char rio_buf[RIO_BUFSIZE];  // internal buffer
} rio_t;

// simplifies calls to bind(), connect(), and accept()
typedef struct sockaddr SA;

/* browser guide
* Firefox..... 1
* Chrome...... 2
* Opera....... 3
* Safari...... 4
*/
typedef struct {
    char filename[512];
    int browser_index;
    off_t offset;              // for support Range
    size_t end;
} http_request;

typedef struct {
    const char *extension;
    const char *mime_type;
} mime_map;


int numberOfLines(char *fileName);

void addNewBrowser(char *browserName);

void addNewIP(char *ip);

void rio_readinitb(rio_t *rp, int fd);

ssize_t written(int fd, void *usrbuf, size_t n);

 static ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n);

ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen);

 void format_size(char* buf, struct stat *stat);

void handle_directory_request(int out_fd, int dir_fd, char *filename);

static const char* get_mime_type(char *filename);

int open_listenfd(int port);

void parse_request(int fd, http_request *req);

void client_error(int fd, int status, char *msg, char *longmsg);

void serve_static(int out_fd, int in_fd, http_request *req, size_t total_size);

void process(int fd, struct sockaddr_in *clientaddr);

int isNumber(char number[]);

int checkargs(int argc, char* argv[]);

void * webBrowser();



#endif // _TRACKER_H 
