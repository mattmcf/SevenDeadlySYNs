#include "FileSystem.h"
#include "../Queue/Queue.h"
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <math.h>
#include <unistd.h>
#include <assert.h>

#define create_new(type) ((type*)malloc(sizeof(type)))
#define max(a,b)	(a > b ? a : b)
#define numDigits(a) (a == 0 ? 1 : (int)(log10(a) + 1))

char* add_strings(int count, ...)
{
    va_list args;
    va_start(args, count);
 
 	int lengths[count];
 	char* strings[count];
 
 	int total = 0;
 	for (int i = 0; i < count; i++)
	{
		strings[i] = (char*)va_arg(args, char*);
		lengths[i] = strlen(strings[i]);
		total += lengths[i];
    }
 
    va_end(args);
	
	char* sum = (char*)malloc(total + 1);
	
	total = 0;
	for (int i = 0; i < count; i++)
	{
		memcpy(sum + total, strings[i], lengths[i]);
		total += lengths[i];
	}
	sum[total] = 0;
	
	return sum;
}
char* copy_string(char* string)
{
	int length = strlen(string);
	char* copy = (char*)malloc(length + 1);
	memcpy(copy, string, length + 1);
	return copy;
}
int unicode_strlen(char* str)
{
	int length = 0;
	while (*str++)
	{
		if ((*str & 0xC0) != 0x80)
		{
			length++;
		}
	}
	return length;
}

// ******************************** File ********************************

typedef struct
{
	char* name;
	unsigned long size;
	time_t last_modified;
} _File;

File* file_new(char* path, char* name)
{
    struct stat st;
    stat(path, &st);
	
	_File* file = create_new(_File);
	file->name = copy_string(name);
	file->size = st.st_size;
	file->last_modified = st.st_mtime;
	
	return (File*)file;
}
void file_destroy(File* file)
{
	_File* f = (_File*)file;
	free(f->name);
	free(f);
	return;
}
char* file_get_name(File* file)
{
	_File* f = (_File*)file;
	return f->name;
}
int file_get_size(File* file)
{
	_File* f = (_File*)file;
	return f->size;
}
void file_print(File* file, int nameWidth, int digitsWidth, int depth)
{
	_File* f = (_File*)file;
		
	for (int i = 0; i < depth; i++)
	{
		printf("  |");
	}
		
	printf("-%-*s %*lu %s", nameWidth, f->name, digitsWidth, f->size, ctime(&(f->last_modified)));
}
File* file_copy(File* file)
{
	_File* f = (_File*)file;
	_File* copy = create_new(_File);
	copy->name = copy_string(f->name);
	copy->size = f->size;
	copy->last_modified = f->last_modified;
	return (File*)copy;
}
int file_equals(void* elementp, void* keyp)
{
	_File* f0 = (_File*)elementp;
	_File* f1 = (_File*)keyp;
	
	return strcmp(f0->name, f1->name) == 0 && f0->size == f1->size && f0->last_modified == f1->last_modified;
}

// ******************************** Folder ********************************

typedef struct
{
	char* name;
	Queue* files;
	Queue* folders;
} _Folder;

Folder* folder_new(char* path, char* name)
{		
	DIR* d = opendir(path);
	if (!d)
	{
		return NULL;
	}
		
	_Folder* folder = create_new(_Folder);
	folder->name = copy_string(name);
	folder->files = queue_new();
	folder->folders = queue_new();
		
    struct dirent *dir;
	while ((dir = readdir(d)) != NULL)
	{
		char* path_to_content = add_strings(3, path, "/", dir->d_name);
		
	    struct stat path_stat;
	    stat(path_to_content, &path_stat);
	
		if (dir->d_name[0] != '.')
		{
		    if (S_ISDIR(path_stat.st_mode))
			{
				Folder* child = folder_new(path_to_content, dir->d_name);
				if (child)
				{
					queue_push(folder->folders, child);
				}
			}
			else
			{
				File* child = file_new(path_to_content, dir->d_name);
				if (child)
				{
					queue_push(folder->files, child);
				}
			}
		}
		
		free(path_to_content);
	}
	
	closedir(d);
	
	return (Folder*)folder;
}
void folder_destroy(Folder* folder)
{
	_Folder* f = (_Folder*)folder;
	free(f->name);
	queue_apply(f->files, (QueueApplyFunction)file_destroy);
	queue_apply(f->folders, (QueueApplyFunction)folder_destroy);
	queue_destroy(f->files);
	queue_destroy(f->folders);
	free(f);
	return;
}
int folder_equals(void* elementp, void* keyp)
{
	_Folder* f0 = (_Folder*)elementp;
	_Folder* f1 = (_Folder*)keyp;
	
	return strcmp(f0->name, f1->name) == 0;
}
void folder_print(Folder* folder, int depth)
{
	for (int i = 0; i < depth; i++)
	{
		printf("  |");
	}
	
	_Folder* f = (_Folder*)folder;
	
	printf("-%s\n", f->name);
	
	int maxLength = 0;
	int maxDigits = 0;
	for (int i = 0; i < queue_length(f->files); i++)
	{
		File* file = queue_get(f->files, i);
		maxLength = max(maxLength, unicode_strlen(file_get_name(file)));
		maxDigits = max(maxDigits, numDigits(file_get_size(file)));
	}
	
	for (int i = 0; i < queue_length(f->files); i++)
	{
		file_print(queue_get(f->files, i), maxLength, maxDigits, depth + 1);
	}
	for (int i = 0; i < queue_length(f->folders); i++)
	{
		folder_print(queue_get(f->folders, i), depth + 1);
	}
} 
Queue* folder_get_folders(Folder* folder)
{
	_Folder* f = (_Folder*)folder;
	return f->folders;
}
Queue* folder_get_files(Folder* folder)
{
	_Folder* f = (_Folder*)folder;
	return f->files;
}
char* folder_get_name(Folder* folder)
{
	_Folder* f = (_Folder*)folder;
	return f->name;
}
Folder* folder_copy(Folder* folder)
{
	_Folder* f = (_Folder*)folder;
	
	_Folder* copy = create_new(_Folder);
	copy->name = copy_string(f->name);
	copy->files = queue_new();
	copy->folders = queue_new();
	
	for (int i = 0; i < queue_length(f->files); i++)
	{
		queue_push(copy->files, file_copy(queue_get(f->files, i)));
	}
	for (int i = 0; i < queue_length(f->folders); i++)
	{
		queue_push(copy->folders, folder_copy(queue_get(f->folders, i)));
	}
	
	return (Folder*)copy;
}
/*
folder = folder - toSubtract
Returns 0 if folder and toSubtract are identical
*/
void folder_subtract(Folder* folder, Folder* toSubtract)
{
	_Folder* f = (_Folder*)folder;
	_Folder* toSub = (_Folder*)toSubtract;
		
	for (int i = 0; i < queue_length(toSub->files); i++)
	{
		File* removed = queue_remove(f->files, file_equals, queue_get(toSub->files, i));
		if (removed)
		{
			file_destroy(removed);
		}
	}
	
	for (int i = 0; i < queue_length(toSub->folders); i++)
	{
		Folder* toRemove = queue_get(toSub->folders, i);
		Folder* removed = queue_search(f->folders, folder_equals, toRemove);
				
		if (removed)
		{			
			folder_subtract(removed, toRemove);
			_Folder* tmp = (_Folder*)removed;
			
			if (queue_length(tmp->files) == 0 && queue_length(tmp->folders) == 0)
			{
				queue_remove(f->folders, folder_equals, toRemove);
				folder_destroy(removed);
			}
		}
	}
}

// ******************************** Filesystem ********************************

typedef struct
{
	char* root_path;
	Folder* root;
} _FileSystem;

FileSystem* filesystem_new(char* path)
{
	_FileSystem* fs = create_new(_FileSystem);
	fs->root_path = copy_string(path);
	fs->root = folder_new(path, "");
	
	return (FileSystem*)fs;
}
void filesystem_destroy(FileSystem* filesystem)
{
	_FileSystem* fs = (_FileSystem*)filesystem;
	free(fs->root_path);
	folder_destroy(fs->root);
	free(fs);
	return;
}
Folder* filesystem_get_root(FileSystem* filesystem)
{
	_FileSystem* fs = (_FileSystem*)filesystem;
	return fs->root;
}
char* filesystem_get_root_path(FileSystem* filesystem)
{
	_FileSystem* fs = (_FileSystem*)filesystem;
	return fs->root_path;
}
void filesystem_subtract(FileSystem* filesystem0, FileSystem* filesystem1)
{
	_FileSystem* fs0 = (_FileSystem*)filesystem0;
	_FileSystem* fs1 = (_FileSystem*)filesystem1;
	
	folder_subtract(fs0->root, fs1->root);
}
FileSystem* filesystem_copy(FileSystem* filesystem)
{
	_FileSystem* fs = (_FileSystem*)filesystem;
	_FileSystem* copy = create_new(_FileSystem);
	copy->root_path = copy_string(fs->root_path);
	copy->root = folder_copy(fs->root);
	return (FileSystem*)copy;
}
void filesystem_print(FileSystem* filesystem)
{
	_FileSystem* fs = (_FileSystem*)filesystem;
	
	printf("Printing filesystem at: %s\n", fs->root_path);
	folder_print(fs->root, 0);
}
void filesystem_diff(FileSystem* filesystem, FileSystem** additions, FileSystem** deletions)
{
	*additions = filesystem_new(((_FileSystem*)filesystem)->root_path);
	*deletions = filesystem_copy(filesystem);
	
	filesystem_subtract(*deletions, *additions);
	filesystem_subtract(*additions, filesystem);
}

// ******************************** Iterator ********************************

typedef struct
{
	FileSystem* fs;
	Queue* folder_stack;
	Queue* path_stack;
	int file_index;
	Queue* current_files;
	char* path;
} _FileSystemIterator;

FileSystemIterator* filesystemiterator_new(FileSystem* fs)
{
	_FileSystemIterator* fsi = create_new(_FileSystemIterator);
	fsi->fs = fs;
	fsi->folder_stack = queue_new();
	fsi->path_stack = queue_new();
	fsi->file_index = 0;
	fsi->path = NULL;
	fsi->current_files = folder_get_files(filesystem_get_root(fs));
	queue_push(fsi->folder_stack, filesystem_get_root(fs));
	queue_push(fsi->path_stack, copy_string(filesystem_get_root_path(fs)));
	return (FileSystemIterator*)fsi;
}
void filesystemiterator_destroy(FileSystemIterator* iterator)
{
	_FileSystemIterator* fsi = (_FileSystemIterator*)iterator;
	
	queue_destroy(fsi->folder_stack);
	queue_apply(fsi->path_stack, free);
	queue_destroy(fsi->path_stack);
	if (fsi->path)
	{
		free(fsi->path);
	}
}

char* filesystemiterator_next(FileSystemIterator* iterator)
{
	_FileSystemIterator* fsi = (_FileSystemIterator*)iterator;
	
	if (fsi->path)
	{
		free(fsi->path);
		fsi->path = NULL;
	}
	
	if (fsi->file_index == queue_length(fsi->current_files))
	{
		Folder* folder = queue_spop(fsi->folder_stack);
		if (folder == NULL)
		{
			return NULL;
		}
		
		char* current_path = queue_spop(fsi->path_stack);
	
		Queue* subfolders = folder_get_folders(folder);			
		for (int i = 0; i < queue_length(subfolders); i++)
		{
			Folder* f = queue_get(subfolders, i);
			queue_push(fsi->folder_stack, f);
			queue_push(fsi->path_stack, add_strings(3, current_path, "/", folder_get_name(f)));
		}
	
		fsi->file_index = 0;
			
		free(current_path);
		
		Folder* next_folder = queue_speek(fsi->folder_stack);
		if (next_folder == NULL)
		{
			return NULL;
		}
		fsi->current_files = folder_get_files(next_folder);
		
		char* path = queue_speek(fsi->path_stack);
		assert(path);
		fsi->path = add_strings(2, path, "/");
		return fsi->path;
	}
	
	char* path = queue_speek(fsi->path_stack);
	assert(path);
	File* file = queue_get(fsi->current_files, fsi->file_index);
	fsi->file_index += 1;
	fsi->path = add_strings(3, path, "/", file_get_name(file));
	return fsi->path;
}

// ******************************** Main ********************************

/*
int main()
{
	FileSystem* fs = filesystem_new("/Users/jacob/Dropbox");
	sleep(10);
	
	FileSystem* additions;
	FileSystem* deletions;
	filesystem_diff(fs, &additions, &deletions);
	printf("Additions:\n");
	filesystem_print(additions);
	
	FileSystemIterator* fsi = filesystemiterator_new(additions);
	char* path;
	while ((path = filesystemiterator_next(fsi)))
	{
		printf("%s\n", path);
	}
	filesystemiterator_destroy(fsi);
	
	printf("\nDeletions:\n");
	filesystem_print(deletions);
	
	fsi = filesystemiterator_new(deletions);
	while ((path = filesystemiterator_next(fsi)))
	{
		printf("%s\n", path);
	}
	filesystemiterator_destroy(fsi);
	
	filesystem_destroy(fs);
	filesystem_destroy(additions);
	filesystem_destroy(deletions);
	
	return 0;
}
*/















