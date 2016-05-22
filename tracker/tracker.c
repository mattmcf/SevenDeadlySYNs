/*
* Dartmouth College CS60 Final Project
* DartSync
*
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
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

#include "tracker.h"
#include "network_tracker.h"
#include "../utility/FileSystem/FileSystem.h"

FileSystem* fs;
peer_table* peerTable;
int peerTableSize = 16;

int main() {
	// create file system
	fs = filesystem_new("~/dartsync");

	// create peer table
	peerTable = createPeerTable();

	// start network thread
	TNT *network = StartTrackerNetwork();

	// main thread
	while(1){

		// If there is a new peer in the Queue
			// get peerID and add peer to peer table
			// get peer file system and reply with diffs from master file system
			// broadcast to all other peers that there is a new peer
		int peerID = receive_new_client(network);
		if(peerID>0){
			if (addPeerToTable(peerID)<0){	// maybe print peer table after this
				printf("failed to add to peer table.\n");
			}
			send_transaction_update(network, &fs, peerID);
			send_peer_added(network);
		}

		// if there is a peer disconnect
			// get peerID and remove peer from peer table
			// broadcast to all otehr peers that peer left
		if(checkForPeerDisconnect()>0){
			// int peerID; // HOW TO GET THIS?
			// removePeerFromTable(peerID); // maybe print peer table after this
			// trkr_network_sendPeerRemovedBroadcast(peerID);
		}

		// if there is a file update
			// take the diff
			// apply diff to local fs
			// broadcast diff to all peers
		if(checkForFileUpdates()>0){
			// FileSystem* diff;
			// get file system
			//broadcast to all cluents
			// trkr_network_sendFileUpdatedBroadcast(fileUpdate);
		}
		if(fileRetrieveSuccess()>0){
			// broadcast out to all peers
		}

		sleep(10);
		printf("Restarting loop.\n");
	}


	filesystem_destroy(fs);
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
	free(deleteTable->peerIDs);
	free(deleteTable);
	return 1;
}

// Takes a new peer and adds them to a table based on unique ID
int addPeerToTable(int peerID) {
	peerTable->numberOfEntries++;
	// find free slot and add peer id
	int i;
	for(i = 0; i < peerTableSize; i++){
		// if peerID is already in table, function fails
		if (peerTable->peerIDs[i] == peerID){
			return -1;
		}
		// if an empty slot is found
		if (peerTable->peerIDs[i] == -1){
			peerTable->peerIDs[i] = peerID;
			peerTable->numberOfEntries++;
			return 1;
		}
	}
	// all slots full. add more slots
	peerTableSize = peerTableSize * 2;
	peerTable->peerIDs = (int*)realloc(peerTable->peerIDs, peerTableSize);
	// save peer as the next index (old peer size + 1)
	i++;
	peerTable->peerIDs[i] = peerID;
	peerTable->numberOfEntries++;
	return 1;
}

// Removes peer after they leave the network
int removePeerFromTable(int peerID){
	int i = 0;
	// either get to end of table or find peer
	while (i < peerTableSize && peerTable->peerIDs[i] != peerID){
		i++;
	}
	// if found peer
	if (peerTable->peerIDs[i] != peerID){
		peerTable->peerIDs[i] = -1;
		peerTable->numberOfEntries--;
		// TODO: LET NETWORK KNOW TO BROADCAST TO EVERYONE
		return 1;
	}
	// could not find peer to remove
	return -1;
}

// Polls queue to see if new peer has tried to connect.
// returns 1 if new peer exists
// returns -1 if no new peer exists
int checkForNewPeer(){
	int peerID = trkr_network_checkNewPeer();
	// if new peer, add peer to table, alert other peers
	if (peerID != -1){
		addPeerToTable(peerID);
		// trkr_network_broadcastNewPeer(peerID);
		return 1;
	}
	return -1;
}

// Polls queue to see if peer has disconnected
// returns 1 if peer left
// returns -1 if no peer has left
int checkForPeerDisconnect(){
	int peerID = trkr_network_checkPeerDisconnect();
	// if disconnect peer, delete from table, alert other peers
	if (peerID != -1){
		removePeerFromTable(peerID);
		// trkr_network_breoadcastPeerLeft(peerID);
		return 1;
	}
	return -1;
}



//close everything, free memory
int closeTracker(){
	destroyPeerTable(peerTable);
	filesystem_destroy(fs);
	return 1;
}



// ****************************************************************
//						Handling File Changes
// ****************************************************************

// identifies files that the peer needs and sends table of files and clients
int sendUpdates(int peerID){
	printf("sendUpdates.\n");
	return 1;
}
/*
compare peer file system to saved system
find difference
send difference
*/

int checkForFileUpdates() {
	if (trkr_network_fileUpdated()>0){
		printf("File was updated by peer.\n");
	}
	return 1;
}

// peer successfully retrieved master. Update file table
int clientFileRetrieveSuccess(int peerID, int fileID){
	printf("Peer successfully retrieved file.\n");
	return 1;
}
/*
update file system and peer table to reflect that the peer now has new file
*/

int fileRetrieveSuccess(){
	printf("Check for file success.\n");
	return 1;
}


// when a file update comes from a peer, this function will handle the update
// int updateFile(int fileID, diff change){
// 	printf("update file.\n");
// 	return 1;
// }
/*
retrieve correct file
apply update
save file
alert peers that update happened
*/
