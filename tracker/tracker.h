/*
* Dartmouth College CS60 Final Project
* DartSync
*




*/

#ifndef _TRACKER_H
#define _TRACKER_H


#include "network_tracker.h"
#include "../utility/FileSystem/FileSystem.h"

typedef struct peer_table {
  int numberOfEntries;		        //peers in network
  int *peerIDs;		//peer table entries
} peer_table;


// ceates a new peer table. allocates the memory
peer_table* createPeerTable();


// destroys peer table and frees memory
int destroyPeerTable(peer_table* deleteTable);

// Takes a new peer and adds them to a table based on unique ID
int addPeerToTable(int peerID);

// Removes peer after they leave the network
int removePeerFromTable(int peerID);

// Polls queue to see if new peer has tried to connect.
// returns 1 if new peer exists
// returns -1 if no new peer exists
int checkForNewPeer();

// Polls queue to see if peer has disconnected
// returns 1 if peer left
// returns -1 if no peer has left
int checkForPeerDisconnect();

//close everything, free memory
int closeTracker();

// peer successfully retrieved master. Update file table
int clientFileRetrieveSuccess(int peerID, int fileID);
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

int fileRetrieveSuccess();

int checkForFileUpdates() ;
// when a file update comes from a peer, this function will handle the update
// int updateFile(int fileID, diff change);
/*
retrieve correct file
apply update
save file
alert peers that update happened
*/

#endif // _TRACKER_H 
