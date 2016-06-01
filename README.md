# SevenDeadlySYNs
Dartmouth CS 60 - Final Project

Adam Grounds
Dave Harmon
Jacob Weiss
Matt McFarland

## To Make Project
Type `make` which will produce the `./client/client_app` and `./tracker/tracker_app` executables.

`make clean` will clean project AND remove `~/dart_sync` if it exists.

## To Start Dart Sync
* Invoke the `./tracker/tracker_app` which will start the tracker executable on the current machine.
* Invoke `./client/client_app [TRACKER_IPv4_ADDRESS] [SYNC_DIRECTORY]` where `SYNC_DIRECTORY` is optional. If left unspecified, the client will use the default directory `~/dart_sync` as the syncronized directory (will create directory if it does not exist).
* To end the tracker executable or client application, use `ctrl+C` to send a kill signal to the process, which will neatly clean up its execution and exit.

## On the "Session Synchronous" nature of Dart Sync
When the client connects to the tracker, it will make all the changes necessary to make it's shared directory folder conform to the master file system held by the tracker. If this means the client must remove the existing contents of the shared directory, it will do so. Be careful! While a client is connected, it's shared directory should be identical to the shared directories of all the other clients. See our documentation page for more information on how this is acheived. 

