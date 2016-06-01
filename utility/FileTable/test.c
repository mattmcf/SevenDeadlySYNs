#include "FileTable.h"
#include "../FileSystem/FileSystem.h"
#include <stdio.h>

int main()
{
	/*
	char* path = "/Users/jacob/dart_sync/";
	printf("Loading filesystem at %s\n", path);
	FileSystem* fs = filesystem_new(path);
	filesystem_print(fs);
	
	FileTable* ft = filetable_new();
	filetable_add_filesystem(ft, fs, 10);
	
	filetable_print(ft);
	
	char* data;
	int length;
	filetable_serialize(ft, &data, &length);
	printf("Serialized %d bytes\n", length);
	
	FileTable* deser = filetable_deserialize(data, &length);
	printf("Deserialized %d bytes\n", length);
	filetable_print(ft);
	*/
	
	FileTable* ft = filetable_new();
	FileSystem* fs1 = filesystem_new("/Users/jacob/dartsync/sdp");
	FileSystem* fs2 = filesystem_new("/Users/jacob/dartsync/testdir");
	FileSystem* fs3 = filesystem_new("/Users/jacob/dartsync/testdir copy");

	filetable_add_filesystem(ft, fs1, 0);
	filetable_add_filesystem(ft, fs2, 1);
	filetable_add_filesystem(ft, fs3, 2);

	FileTableIterator * fti1 = filetableiterator_new(ft);
	char * path;
	int i = 0;
	while ( (path = filetableiterator_path_next(fti1)) != NULL) {
		filetable_set_that_peer_has_file_chunk(ft, path, i % 3, 0);
		filetable_set_that_peer_has_file_chunk(ft, path, (i + 1) % 3, 0);
	}
	filetableiterator_destroy(fti1);
	
	char* data;
	int length;
		
	filetable_serialize(ft, &data, &length);
	
	printf("%d\n", length);
	
	FileTable* ftds = filetable_deserialize(data, &length);
	
	printf("%d\n", length);
	
	filetable_print(ftds);

	filetable_remove_peer(ftds, 2);

	filetable_print(ftds);

	// FileTableIterator * fti = filetableiterator_new(ftds);
	// char * path;
	// int j = 0;
	// while ( (path = filetableiterator_path_next(fti)) != NULL) {
	// 	printf("%d: %s\n", i++, path);
	// }
	// filetableiterator_destroy(fti);

	return 0;
}