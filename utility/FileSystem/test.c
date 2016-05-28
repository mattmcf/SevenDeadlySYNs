// ******************************** Main ********************************

#include <stdio.h>
#include <unistd.h>
#include "FileSystem.h"

int main()
{
	printf("Testing serialization and deserialization...\n");
	
	char* path = "/Users/jacob/dart_sync/";
	printf("Loading filesystem at %s\n", path);
	FileSystem* fs = filesystem_new(path);
	filesystem_print(fs);
	
	char* serializedFs;
	int length;
	printf("Serializing!\n");
	filesystem_serialize(fs, &serializedFs, &length);
	printf("Serialized %d bytes!\n", length);
	printf("Deserializing!\n");
	FileSystem* deserialized = filesystem_deserialize(serializedFs, &length);
	printf("Deserialized %d bytes!\n", length);
	filesystem_print(deserialized);
	
	FileSystem* additions;
	FileSystem* deletions;
	filesystem_diff(fs, deserialized, &additions, &deletions);
	printf("Printing differences. (There should be no differences)\n");
	filesystem_print(additions);
	filesystem_print(deletions);
	
	filesystem_destroy(additions);
	filesystem_destroy(deletions);
	filesystem_destroy(deserialized);
	
		
	printf("\n\n*****************************\n\nTesting diff detection. Make changes in next 5 seconds.\n");
	sleep(10);
		
	FileSystem* changes = filesystem_new(path);
	filesystem_diff(fs, changes, &additions, &deletions);
	printf("Additions\n");
	filesystem_print(additions);
	printf("Deletions\n");
	filesystem_print(deletions);
	filesystem_get_updates(additions, deletions);
	printf("Additions\n");
	filesystem_print(additions);
	printf("Deletions\n");
	filesystem_print(deletions);
	
	FileSystem* withChanges = filesystem_copy(fs);
	filesystem_minus_equals(withChanges, deletions);	
	filesystem_plus_equals(withChanges, additions);
	
	printf("Old + additions - deletions\n");
	filesystem_print(withChanges);
	
	filesystem_destroy(additions);
	filesystem_destroy(deletions);
	
	filesystem_diff(changes, withChanges, &additions, &deletions);
	
	printf("Printing differences. (There should be no differences)\n");
	filesystem_print(additions);
	filesystem_print(deletions);
	
	return 0;
}
