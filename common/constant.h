// common/constants.h
//
// defines constants for communication between tracker and peers

#ifndef _CONSTANTS_H
#define _CONSTANTS_H

// new clients connect here on tracker
#define TRACKER_LISTENING_PORT 5969 

// clients listen here for connections from other clients
#define CLIENT_LISTENING_PORT 7681

// minimum time (seconds) that a client needs to talk to tracker before being considered dead
#define DIASTOLE 10 

// time spent waiting for sockets to receive data before polling queues
#define NETWORK_WAIT 3

// defined DART_SYNC directory that will be syncronized across peers
#define DARTSYNC_DIR "~/dart_sync/"

// metadata file that contains the most recent serialized 
// version of the filesystem received from the tracker
#define DARTSYNC_METADATA "~/.dart_sync_metadata.txt"

#endif // _CONSTANTS_H
