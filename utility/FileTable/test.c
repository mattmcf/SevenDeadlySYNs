#include "FileTable.h"
#include "../FileSystem/FileSystem.h"
#include <stdio.h>

int main()
{
	char* path = "/Users/jacob/dart_sync/";
	printf("Loading filesystem at %s\n", path);
	FileSystem* fs = filesystem_new(path);
	filesystem_print(fs);
	
	FileTable* ft = filetable_new();
	filetable_add_filesystem(ft, fs, 10);
	
	filetable_print(ft);
	
	return 0;
}