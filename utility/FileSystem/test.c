// ******************************** Main ********************************

#include <stdio.h>
#include <unistd.h>
#include "FileSystem.h"
#include "../ColoredPrint/ColoredPrint.h"
#include <stdlib.h>

int main()
{
	FORMAT_ARG args[] = {COLOR_L_BLUE, COLOR_UNDERLINE, 0};
	int blueid = register_format(args);
		
	
	printf("Testing serialization and deserialization...\n");
	
	char* path = "/Users/jacob/dartsync/";
	printf("Loading filesystem at %s\n", path);
	FileSystem* fs = filesystem_new(path);
	
	printf("\n\n*****************************\n\nTesting Iterator\n");
	
	for (int i = 0; i < 2; i++)
	{
		FileSystemIterator* fsi = filesystemiterator_new(fs, 0);
		char* ipath = NULL;
		int ilength;
		time_t imod_time;
		while ((ipath = filesystemiterator_next(fsi, &ilength, &imod_time)))
		{
			if (ilength < 0)
			{
				format_printf(blueid, "%s\n", ipath);
			}
			else
			{
				printf("%s\n", ipath);
			}
		}
		filesystemiterator_destroy(fsi);
		
		if (i == 0)
		{
			printf("Removing /sdp/license.txt\n");
			filesystem_remove_file_at_path(fs, "/sdp/license.txt");
			printf("\n");
		}
	}
	printf("\n\n*****************************\n\nTesting Serialization\n");
	
	
	char* serializedFs;
	int length;
	printf("Serializing!\n");
	filesystem_serialize(fs, &serializedFs, &length);
	printf("Serialized %d bytes!\n", length);
	printf("Deserializing!\n");
	FileSystem* deserialized = filesystem_deserialize(serializedFs, &length);
	printf("Deserialized %d bytes!\n", length);
	
	FileSystem* additions;
	FileSystem* deletions;
	
	if (filesystem_equals(fs, deserialized))
	{
		printf("Serialization test passed!\n");
	}
	else
	{
		printf("Serialization test failed!\n");
	
		filesystem_diff(fs, deserialized, &additions, &deletions);
		printf("Printing differences. (There should be no differences)\n");
		filesystem_print(additions);
		filesystem_print(deletions);
	
		filesystem_destroy(additions);
		filesystem_destroy(deletions);
		filesystem_destroy(deserialized);
	}
	
		
	printf("\n\n*****************************\n\nTesting diff detection. Make changes in next 5 seconds.\n");
	sleep(5);
		
	FileSystem* changes = filesystem_new(path);
	
	if (filesystem_equals(changes, fs))
	{
		printf("Filesystem has not changed!\n");
	}
	else
	{
		filesystem_diff(fs, changes, &additions, &deletions);
		printf("Additions\n");
		filesystem_print_list(additions);
		printf("Deletions\n");
		filesystem_print_list(deletions);
	
		printf("Testing additive change propogation\n");
		FileSystem* withChanges = filesystem_copy(fs);
		filesystem_minus_equals(withChanges, deletions);	
		filesystem_plus_equals(withChanges, additions);
	
		printf("Old - deletions + additions\n");
		filesystem_print(withChanges);
	
		filesystem_destroy(additions);
		filesystem_destroy(deletions);
	
		filesystem_diff(changes, withChanges, &additions, &deletions);
	
		printf("Printing differences. (There should be no differences)\n");
		filesystem_print(additions);
		filesystem_print(deletions);
	}
	
	return 0;
}
