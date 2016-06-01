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

	filetable_add_filesystem(ft, fs1, 10);
	filetable_add_filesystem(ft, fs2, 11);
	filetable_add_filesystem(ft, fs3, 12);

	filetable_add_filesystem(ft, fs1, 11);
	filetable_add_filesystem(ft, fs2, 12);
	filetable_add_filesystem(ft, fs3, 10);
	
	char* data;
	int length;
		
	filetable_serialize(ft, &data, &length);
	
	printf("%d\n", length);
	
	FileTable* ftds = filetable_deserialize(data, &length);
	
	printf("%d\n", length);
	
	//filetable_print(ftds);

	filetable_remove_peer(ft, 12);

	filetable_print(ftds);

	FileTableIterator * fti = filetableiterator_new(ftds);

	char * path;
	int i = 0;
	while ( (path = filetableiterator_path_next(fti)) != NULL) {
		printf("%d: %s\n", i++, path);
	}
	
	filetableiterator_destroy(fti);

	return 0;
}