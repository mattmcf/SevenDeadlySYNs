/*
* Dartmouth College CS60 Final Project
* DartSync
*
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "tracker.h"
#include "../common/constant.h"
#include "network_tracker.h"
#include "../utility/FileSystem/FileSystem.h"

FileSystem* fs;
peer_table* peerTable;
int peerTableSize = 16;
static volatile int keepRunning = 1;

void intHandler(int dummy) {
    keepRunning = 0;
}

int main() {
	signal(SIGINT, intHandler);
	int peerID = -1;
	// create file system
	fs = filesystem_new(DARTSYNC_DIR);
	filesystem_print(fs);

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
		printf("checking for new client\n");
		peerID = -1;
		peerID = receive_new_client(network);
		if(peerID>0){
			if (addPeerToTable(peerID)<0){	// maybe print peer table after this
				printf("Failed to add peer %d to peer table.\n", peerID);
			}
			// send_transaction_update(network, &fs, peerID);
			printf("Send peer added\n");
			send_peer_added(network, peerID); // PROPBABLY NEED SOMETHING TO LET EVERYONE KNOW WHAT PEER ADDED
		}

		// If a peer requests master
			// send master
		printf("checking for master request\n");
		peerID = -1;
		peerID = receive_master_request(network);
		if(peerID>0){
			if(send_master(network, peerID, fs)<0){
				printf("Failed to send master to peer %d\n", peerID);
			}
			printf("Sent master to peer %d\n", peerID);
		}

		// if there is a peer disconnect
			// get peerID and remove peer from peer table
			// broadcast to all otehr peers that peer left
		printf("checking for lost client\n");
		peerID = -1;
		peerID = receive_lost_client(network);
		if(peerID>0){
			if(removePeerFromTable(peerID)<0){
				printf("Failed to remove peer %d from table.\n", peerID);
			}
			printf("Removed peer %d from table.\n", peerID);
			printf("Send peer removed\n");
			send_peer_removed(network); // PROPBABLY NEED SOMETHING TO LET EVERYONE KNOW WHAT PEER DROPPED
		}

		// if there is a file update
			// take the diff
			// apply diff to local fs
			// broadcast diff to all peers
		FileSystem *peerFileSystem = receive_client_update(network, int *clientID);
		if(peerFileSystem != NULL){
			// ASSUMPTION: updates will happen in pairs and will be added to queues in pairs

			FileSystem *additions;
			FileSystem *deletions;
			filesystem_diff(fs, peerFileSystem, &additions, &deletions)
			filesystem_minus_equals(fs, deletions);	
			filesystem_plus_equals(fs, additions);
			send_FS_update(network); // NEED SOME WAY TO SEND THE DIFF AND LET THEM KNOW WHO TO REQUEST FROM 
			send_FS_update(network); // NEED SOME WAY TO SEND THE DIFF AND LET THEM KNOW WHO TO REQUEST FROM 
		}

		// if(fileRetrieveSuccess()>0){
		// 	// broadcast out to all peers
		// }

		sleep(3);
		printf("Restarting loop.\n");
	}

	if (closeTracker()<0) {
	 	printf("Failed to close everything. Quitting anyway.\n");
	} 
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

// // Polls queue to see if new peer has tried to connect.
// // returns 1 if new peer exists
// // returns -1 if no new peer exists
// int checkForNewPeer(){
// 	int peerID = trkr_network_checkNewPeer();
// 	// if new peer, add peer to table, alert other peers
// 	if (peerID != -1){
// 		addPeerToTable(peerID);
// 		// trkr_network_broadcastNewPeer(peerID);
// 		return 1;
// 	}
// 	return -1;
// }

// // Polls queue to see if peer has disconnected
// // returns 1 if peer left
// // returns -1 if no peer has left
// int checkForPeerDisconnect(){
// 	int peerID = trkr_network_checkPeerDisconnect();
// 	// if disconnect peer, delete from table, alert other peers
// 	if (peerID != -1){
// 		removePeerFromTable(peerID);
// 		// trkr_network_breoadcastPeerLeft(peerID);
// 		return 1;
// 	}
// 	return -1;
// }

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
int newPeerBroadcast(int newPeerID){
	for (int i = 0; i < peerTableSize; i++){
		if(peerTable->peerIDs[i] != -1 && peerTable->peerIDs[i] != newPeerID){
			if(send_peer_added(network, newPeerID)<0){ //HOW DO WE INDICATE WHAT PEER SHOULD RECEIVE
				printf("Failed to send new peer update to peer %d\n", peerTable->peerIDs[i]);
			}
		}
	}
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

// int checkForFileUpdates() {
// 	if (trkr_network_fileUpdated()>0){
// 		printf("File was updated by peer.\n");
// 	}
// 	return 1;
// }

// // peer successfully retrieved master. Update file table
// int clientFileRetrieveSuccess(int peerID, int fileID){
// 	printf("Peer successfully retrieved file.\n");
// 	return 1;
// }
/*
update file system and peer table to reflect that the peer now has new file
*/

// int fileRetrieveSuccess(){
// 	printf("Check for file success.\n");
// 	return 1;
// }


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



