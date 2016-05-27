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
#include <wordexp.h>

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
	if (!string)
	{
		return NULL;
	}
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
	int is_folder;
} _File;

File* file_new(char* path, char* name)
{
    struct stat st;
    stat(path, &st);
	
	_File* file = create_new(_File);
	file->name = copy_string(name);
	file->size = st.st_size;
	file->last_modified = st.st_mtime;
	file->is_folder = 0;
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
		
		
	if (!(f->is_folder))
	{
		for (int i = 0; i < depth; i++)
		{
			printf("  |");
		}
		printf("-%-*s %*lu %s", nameWidth, f->name, digitsWidth, f->size, ctime(&(f->last_modified)));
	}
}
File* file_copy(File* file)
{
	_File* f = (_File*)file;
	_File* copy = create_new(_File);
	copy->name = copy_string(f->name);
	copy->size = f->size;
	copy->last_modified = f->last_modified;
	copy->is_folder = f->is_folder;
	return (File*)copy;
}
int file_equals(void* elementp, void* keyp)
{
	_File* f0 = (_File*)elementp;
	_File* f1 = (_File*)keyp;
	
	return strcmp(f0->name, f1->name) == 0 && f0->size == f1->size && f0->last_modified == f1->last_modified && (f0->is_folder == f1->is_folder);
}
int file_ignore_time(void* elementp, void* keyp)
{
	_File* f0 = (_File*)elementp;
	_File* f1 = (_File*)keyp;
	
	return strcmp(f0->name, f1->name) == 0 && f0->size == f1->size && (f0->is_folder == f1->is_folder);
}
int file_older(void* elementp, void* keyp)
{
	_File* f0 = (_File*)elementp;
	_File* f1 = (_File*)keyp;
	
	return strcmp(f0->name, f1->name) == 0 && f0->last_modified <= f1->last_modified && (f0->is_folder == f1->is_folder);
}
int file_newer(void* elementp, void* keyp)
{
	_File* f0 = (_File*)elementp;
	_File* f1 = (_File*)keyp;
	
	return strcmp(f0->name, f1->name) == 0 && f0->last_modified > f1->last_modified && (f0->is_folder == f1->is_folder);
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
		fprintf(stderr, "Bad folder path\n");
		assert(0);
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
				_Folder* child = (_Folder*)folder_new(path_to_content, dir->d_name);
				if (child)
				{
					queue_push(folder->folders, child);
					_File* file = create_new(_File);
					file->name = copy_string(child->name);
					file->size = 0;
					file->last_modified = 0;
					file->is_folder = 1;
					queue_push(folder->files, file);
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
	_Folder* f = (_Folder*)folder;
		
	for (int i = 0; i < depth; i++)
	{
		printf("  |");
	}
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
*/
void folder_subtract(Folder* folder, Folder* toSubtract, QueueSearchFunction equalsFunc)
{
	_Folder* f = (_Folder*)folder;
	_Folder* toSub = (_Folder*)toSubtract;
		
	for (int i = 0; i < queue_length(toSub->files); i++)
	{
		File* removed = queue_remove(f->files, equalsFunc, queue_get(toSub->files, i));
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
			folder_subtract(removed, toRemove, equalsFunc);
			_Folder* tmp = (_Folder*)removed;
			
			if (queue_length(tmp->files) == 0 && queue_length(tmp->folders) == 0)
			{
				queue_remove(f->folders, folder_equals, toRemove);
				folder_destroy(removed);
			}
		}
	}
}


/*
folder = folder - toSubtract
*/
void folder_remove(Folder* folder, Folder* toSubtract)
{
	_Folder* f = (_Folder*)folder;
	_Folder* toSub = (_Folder*)toSubtract;
		
	for (int i = 0; i < queue_length(toSub->files); i++)
	{
		_File* removedFile = queue_remove(f->files, file_equals, queue_get(toSub->files, i));
		if (removedFile)
		{
			if (removedFile->is_folder)
			{
				_Folder toRemove;
				toRemove.name = removedFile->name;
				Folder* removedFolder = queue_remove(f->folders, folder_equals, &toRemove);
				if (removedFolder)
				{
					folder_destroy(removedFolder);
				}
			}
			file_destroy((File*)removedFile);
		}
	}
	
	for (int i = 0; i < queue_length(toSub->folders); i++)
	{
		Folder* toRemove = queue_get(toSub->folders, i);
		Folder* removed = queue_search(f->folders, folder_equals, toRemove);
				
		if (removed)
		{			
			folder_subtract(removed, toRemove, folder_equals);
		}
	}
}
/*
folder = folder + toAdd
*/
void folder_add(Folder* folder, Folder* toPlus)
{
	_Folder* f = (_Folder*)folder;
	_Folder* toAdd = (_Folder*)toPlus;
		
	for (int i = 0; i < queue_length(toAdd->files); i++)
	{
		_File* fileToAdd = queue_get(toAdd->files, i);
		
		if (fileToAdd->is_folder)
		{
			_Folder search;
			search.name = fileToAdd->name;
			_Folder* folderToAdd = queue_search(toAdd->folders, folder_equals, &search);
			assert(folderToAdd);
			
			_File* fAlreadyContainsFile = queue_search(f->files, file_equals, fileToAdd);
			_Folder* fAlreadyContainsFolder = queue_search(f->folders, folder_equals, &search);
			
			if (fAlreadyContainsFile)
			{
				assert(fAlreadyContainsFolder);
				folder_add((Folder*)fAlreadyContainsFolder, (Folder*)folderToAdd);
			}
			else
			{
				queue_push(f->files, file_copy((File*)fileToAdd));
				_Folder* newFolder = create_new(_Folder);
				newFolder->name = copy_string(fileToAdd->name);
				newFolder->files = queue_new();
				newFolder->folders = queue_new();
				queue_push(f->folders, newFolder);
			}			
		}
		else
		{
			_File* fileExists = queue_search(f->files, file_equals, fileToAdd);
			if (!fileExists)
			{
				queue_push(f->files, file_copy((File*)fileToAdd));
			}
		}
	}
	for (int i = 0; i < queue_length(toAdd->folders); i++)
	{
		_Folder* folderToAdd = queue_get(toAdd->folders, i);
		_Folder* found = queue_search(f->folders, folder_equals, folderToAdd);
		assert(found);
		folder_add((Folder*)found, (Folder*)folderToAdd);
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
	assert(path);
	
    wordexp_t exp_result;
    wordexp(path, &exp_result, 0);
	_FileSystem* fs = create_new(_FileSystem);
	fs->root_path = copy_string(exp_result.we_wordv[0]);
	fs->root = folder_new(exp_result.we_wordv[0], "");
	assert(fs->root);
		
	return (FileSystem*)fs;
}
void filesystem_destroy(FileSystem* filesystem)
{
	_FileSystem* fs = (_FileSystem*)filesystem;
	assert(fs);
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
	assert(fs);
	return fs->root_path ? fs->root_path : "";
}
void filesystem_set_root_path(FileSystem* filesystem, char* path)
{
	_FileSystem* fs = (_FileSystem*)filesystem;
	assert(fs);
	fs->root_path = copy_string(path);
}
FileSystem* filesystem_copy(FileSystem* filesystem)
{
	assert(filesystem);
	_FileSystem* fs = (_FileSystem*)filesystem;
	_FileSystem* copy = create_new(_FileSystem);
	copy->root_path = copy_string(fs->root_path);
	copy->root = folder_copy(fs->root);
	return (FileSystem*)copy;
}
void filesystem_print(FileSystem* filesystem)
{
	assert(filesystem);
	_FileSystem* fs = (_FileSystem*)filesystem;
	
	printf("Printing filesystem at: %s\n", fs->root_path == NULL ? "NULL" : fs->root_path);
	assert(fs->root);
	folder_print(fs->root, 0);
}
void filesystem_minus_equals_diff(FileSystem* filesystem0, FileSystem* filesystem1, QueueSearchFunction equalsFunc)
{
	_FileSystem* fs0 = (_FileSystem*)filesystem0;
	_FileSystem* fs1 = (_FileSystem*)filesystem1;
	
	folder_subtract(fs0->root, fs1->root, equalsFunc);
}
void filesystem_minus_equals(FileSystem* filesystem0, FileSystem* filesystem1)
{
	_FileSystem* fs0 = (_FileSystem*)filesystem0;
	_FileSystem* fs1 = (_FileSystem*)filesystem1;

	assert(fs0 && fs1 && fs0->root && fs1->root);
	
	folder_remove(fs0->root, fs1->root);
}
void filesystem_plus_equals(FileSystem* filesystem0, FileSystem* filesystem1)
{
	_FileSystem* fs0 = (_FileSystem*)filesystem0;
	_FileSystem* fs1 = (_FileSystem*)filesystem1;

	assert(fs0 && fs1 && fs0->root && fs1->root);

	folder_add(fs0->root, fs1->root);
}
void filesystem_diff_helper(FileSystem* old, FileSystem* new, FileSystem** additions, FileSystem** deletions, QueueSearchFunction equalsFunc)
{
	assert(old && new && additions && deletions);
	
	*additions = filesystem_copy(old);
	*deletions = filesystem_copy(new);
	
	filesystem_minus_equals_diff(*deletions, *additions, equalsFunc);
	filesystem_minus_equals_diff(*additions, new, equalsFunc);
	
	FileSystem* tmp = *additions;
	*additions = *deletions;
	*deletions = tmp;
}
void filesystem_handle_updates(FileSystem* additions, FileSystem* deletions)
{
	assert(additions && deletions);
	
	FileSystem* a1;
	FileSystem* d1;
	
	filesystem_diff_helper(additions, deletions, &a1, &d1, file_ignore_time);
		
	FileSystem* a0 = filesystem_copy(additions);
	FileSystem* d0 = filesystem_copy(deletions);
	
	filesystem_minus_equals_diff(a0, d1, file_equals);
	filesystem_minus_equals_diff(d0, a1, file_equals);
	
	filesystem_destroy(a1);
	filesystem_destroy(d1);
		
	filesystem_minus_equals_diff(additions, d0, file_newer);
	filesystem_minus_equals_diff(deletions, a0, file_older);
			
	filesystem_destroy(a0);
	filesystem_destroy(d0);
}
void filesystem_diff(FileSystem* old, FileSystem* new, FileSystem** additions, FileSystem** deletions)
{
	filesystem_diff_helper(old, new, additions, deletions, file_equals);
	filesystem_handle_updates(*additions, *deletions);
}

#define FILE_MARKER (0xFF)
#define FOLDER_START (0xFE)
#define FOLDER_END (0xFD)

void push_string(Queue* queue, char* str)
{
	for (char* ptr = str; *ptr; ptr++)
	{
		queue_push(queue, (void*)(long)(*ptr));
	}
	queue_push(queue, (void*)0);
}
void filesystem_serialize_helper(_Folder* folder, Queue* data)
{
	queue_push(data, (void*)FOLDER_START);
	push_string(data, folder->name);
	for (int i = 0; i < queue_length(folder->files); i++)
	{
		_File* file = queue_get(folder->files, i);
		queue_push(data, (void*)FILE_MARKER);
		push_string(data, file->name);
		char buf[128];
		sprintf(buf, "%ld %ld %d", file->size, file->last_modified, file->is_folder);
		push_string(data, buf);
	}
	for (int i = 0; i < queue_length(folder->folders); i++)
	{
		_Folder* f = queue_get(folder->folders, i);
		filesystem_serialize_helper(f, data);
	}
	queue_push(data, (void*)FOLDER_END);
}

void filesystem_serialize(FileSystem* filesystem, char** data, int* length)
{
	assert(filesystem && data && length);
	_FileSystem* fs = (_FileSystem*)filesystem;
	Queue* buffer = queue_new();
	filesystem_serialize_helper((_Folder*)fs->root, buffer);
	
	int l = queue_length(buffer);
	char* serialized = (char*)malloc(l * sizeof(char));
	for (int i = 0; i < l; i++)
	{
		serialized[i] = (char)queue_get(buffer, i);
	}
	
	queue_destroy(buffer);
	
	*data = serialized;
	*length = l;
}

_Folder* filesystem_deserialize_helper(char** data)
{	
	_Folder* folder = create_new(_Folder);
	folder->files = queue_new();
	folder->folders = queue_new();
	folder->name = copy_string(*data);
	
	*data += strlen(folder->name) + 1;
	
	while (1)
	{
		switch ((unsigned char)(**data))
		{
			case FOLDER_START:
			{
				*data += 1;
				queue_push(folder->folders, filesystem_deserialize_helper(data));
				break;
			}
			case FILE_MARKER:
			{
				*data += 1;
				_File* file = create_new(_File);
				file->name = copy_string(*data);
				*data += strlen(file->name) + 1;
				int numRead;
				sscanf(*data, "%ld %ld %d%n", &(file->size), &(file->last_modified), &(file->is_folder), &numRead);
				queue_push(folder->files, file);
				*data += numRead + 1;
				break;
			}
			case FOLDER_END:
			{
				*data += 1;
				return folder;
				break;
			}
			default:
			{
				assert(0);
			}
		}
	}
}

FileSystem* filesystem_deserialize(char* data, int* bytesRead)
{
	assert(data && bytesRead);
	_FileSystem* fs = create_new(_FileSystem);
	fs->root_path = NULL;
	char* d = data + 1;
	fs->root = (Folder*)filesystem_deserialize_helper(&d);
	*bytesRead = d - data; // Adding one above actually fixes the off by one error
	
	return (FileSystem*)fs;
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
	assert(fs);
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
	assert(iterator);
	_FileSystemIterator* fsi = (_FileSystemIterator*)iterator;
	
	queue_destroy(fsi->folder_stack);
	queue_apply(fsi->path_stack, free);
	queue_destroy(fsi->path_stack);
	if (fsi->path)
	{
		free(fsi->path);
	}
}

char* filesystemiterator_next(FileSystemIterator* iterator, int* length)
{
	_FileSystemIterator* fsi = (_FileSystemIterator*)iterator;
	assert(fsi);
	
	// If there is a path that has been set, then free it.
	if (fsi->path)
	{
		free(fsi->path);
		fsi->path = NULL;
	}
	
	// If the we have iterated through all the files in the current directory...
	if (fsi->file_index == queue_length(fsi->current_files))
	{
		// Get the next folder from the folder stack. Return if it is NULL.
		Folder* folder = queue_spop(fsi->folder_stack);
		if (folder == NULL)
		{
			*length = -1;
			return NULL;
		}
		
		char* current_path = queue_spop(fsi->path_stack);
		
		// Iterate through the subfolders of the next folder and push them onto the stack
		Queue* subfolders = folder_get_folders(folder);			
		for (int i = 0; i < queue_length(subfolders); i++)
		{
			Folder* f = queue_get(subfolders, i);
			queue_push(fsi->folder_stack, f);
			queue_push(fsi->path_stack, add_strings(3, current_path, "/", folder_get_name(f)));
		}
	
		// Reset the file index
		fsi->file_index = 0;
		
		free(current_path);
		
		Folder* next_folder = queue_speek(fsi->folder_stack);
		if (next_folder == NULL)
		{
			*length = -1;
			return NULL;
		}
		fsi->current_files = folder_get_files(next_folder);
		
		*length = -1;
		fsi->path = NULL;
		return filesystemiterator_next(iterator, length);
	}
	
	char* path = queue_speek(fsi->path_stack);
	assert(path);
	File* file = queue_get(fsi->current_files, fsi->file_index);
	fsi->file_index += 1;
	fsi->path = add_strings(3, path, "/", file_get_name(file));
	if (((_File*)file)->is_folder)
	{
		*length = -1;
	}
	else
	{
		*length = (int)(((_File*)file)->size);
	}
	return fsi->path;
}

// ******************************** Main ********************************

/*
int main()
{
	printf("Testing serialization and deserialization...\n");
	
	char* path = "/Users/jacob/Downloads/a4handout";
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
	sleep(5);
		
	FileSystem* changes = filesystem_new(path);
	filesystem_diff(fs, changes, &additions, &deletions);
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
*/


































