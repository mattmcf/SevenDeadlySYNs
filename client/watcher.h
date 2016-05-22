// watcher.h
//
// defines functions for invoking a thread that will get notified whenever files in the file system are altered by user

// 
int init_watcher();

// adds a watch on a directory (should be called on all subdirectories)
int add_watch(char * dir_name);
