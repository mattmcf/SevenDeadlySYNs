#include <stdlib.h>
#include <string.h>
#include "FileTable.h"
#include "../HashTable/HashTable.h"
#include "../Queue/Queue.h"
#include "../ChunkyFile/ChunkyFile.h"
#include <assert.h>
#include <stdio.h>

typedef struct
{
	HashTable* table;
} _FileTable;

typedef struct
{
	char* path;
	Queue* chunks;
} FileTableEntry;

void filetableentry_destroy(FileTableEntry* fte)
{
	queue_apply(fte->chunks, (QueueApplyFunction)queue_destroy);
	queue_destroy(fte->chunks);
	free(fte->path);
	free(fte);
}
	

/* Peter Weinberger's hash function, from Aho, Sethi, & Ullman
 * p. 436. */
int hashPJW(char *s)
{
	unsigned h = 0, g;
	char *p;

	for (p = s; *p != '\0'; p++)
	{
		h = (h << 4) + *p;
		if ((g = (h & 0xf0000000)) != 0)
		{
			h ^= (g >> 24) ^ g;
		}
	}

	return h;
}

int filetableentry_hash(FileTableEntry* element)
{
	return hashPJW(element->path);
}

int filetableentry_equals(FileTableEntry* e0, FileTableEntry* e1)
{
	return strcmp(e0->path, e1->path) == 0;
}

FileTable* filetable_new()
{
	_FileTable* ft = (_FileTable*)malloc(sizeof(_FileTable*));
	ft->table = hashtable_new((HashFunction)filetableentry_hash, (ElementEqualsFunction)filetableentry_equals);
	return (FileTable*)ft;
}

void filetable_destroy(FileTable* filetable);

void filetable_add_filesystem(FileTable* filetable, FileSystem* filesystem, int peer)
{
	_FileTable* ft = (_FileTable*)filetable;
	FileSystemIterator* fsi = filesystemiterator_new(filesystem);
	
	FileTableEntry fte;
	int length;
	while ((fte.path = filesystemiterator_next(fsi, &length)))
	{
		if (length < 0)
		{
			continue;
		}
		
		FileTableEntry* entry = hashtable_get_element(ft->table, &fte);
		if (entry)
		{
			filetableentry_destroy(hashtable_remove_element(ft->table, &fte));
		}
		
		int numChunks = num_chunks_for_size(length);
		
		FileTableEntry* newEntry = (FileTableEntry*)malloc(sizeof(FileTableEntry));
		newEntry->path = copy_string(fte.path);
		newEntry->chunks = queue_new();
		for (int i = 0; i < numChunks; i++)
		{
			Queue* q = queue_new();
			queue_push(q, (void*)(long)peer);
			queue_push(newEntry->chunks, q);
		}
		hashtable_add(ft->table, newEntry);
	}
	
	filesystemiterator_destroy(fsi);
}

void filetable_remove_filesystem(FileTable* filetable, FileSystem* filesystem)
{
	_FileTable* ft = (_FileTable*)filetable;
	FileSystemIterator* fsi = filesystemiterator_new(filesystem);
	
	FileTableEntry fte;
	int length;
	while ((fte.path = filesystemiterator_next(fsi, &length)))
	{
		if (length < 0)
		{
			continue;
		}
		
		FileTableEntry* entry = hashtable_get_element(ft->table, &fte);
		if (entry)
		{
			filetableentry_destroy(hashtable_remove_element(ft->table, &fte));
		}		
	}
	filesystemiterator_destroy(fsi);
}

void filetable_serialize(FileTable* filetable, char** data, int* length);
FileTable* filetable_deserialize(char* data, int* bytesRead);

Queue* filetable_get_peers_who_have_file_chunk(FileTable* filetable, char* path, int chunk)
{
	_FileTable* ft = (_FileTable*)filetable;
	
	FileTableEntry search;
	search.path = path;
	FileTableEntry* fte = hashtable_get_element(ft->table, &search);
	
	if (fte && queue_length(fte->chunks) < chunk)
	{
		return queue_get(fte->chunks, chunk);
	}
	else
	{
		return NULL;
	}
}

int filetable_peer_search(int a, int b)
{
	return a == b;
}

void filetable_set_that_peer_has_file_chunk(FileTable* filetable, char* path, int peer, int chunkNum)
{
	Queue* chunk = filetable_get_peers_who_have_file_chunk(filetable, path, chunkNum);
	
	if (!chunk)
	{
		printf("Trying to access chunk of file that does not exist\n");
		assert(0);
	}
	
	for (int i = 0; i < queue_length(chunk); i++)
	{
		if ((int)(long)queue_get(chunk, i) == peer)
		{
			printf("Trying to add peer to chunk that peer already has\n");
			assert(0);
		}
	}
	queue_push(chunk, (void*)(long)peer);
}

int remove_peer(int elementp, int userData)
{
	return elementp != userData;
}

void  filetable_remove_peer(FileTable* filetable, int id)
{
	_FileTable* ft = (_FileTable*)filetable;
	HashTableIterator* hti = hashtableiterator_new(ft->table);
	if (!hti)
	{
		return;
	}
	
	FileTableEntry* fte;
	while ((fte = hashtableiterator_next(hti)))
	{
		for (int i = 0; i < queue_length(fte->chunks); i++)
		{
			Queue* old = queue_get(fte->chunks, i);
			queue_set(fte->chunks, queue_filter(old, (QueueFilterFunction)filetable_remove_peer, (void*)(long)id), id);
			queue_destroy(old);
		}
	}
	hashtableiterator_destroy(hti);
}

void filetable_print_id(int id)
{
	printf("%d ", id);
}
void filetable_print_chunk(Queue* chunk)
{
	printf("[ ");
	queue_apply(chunk, (QueueApplyFunction)filetable_print_id);
	printf("]\n");
}
void filetable_print_file(FileTableEntry* entry)
{
	printf("%s\n", entry->path);	
	for (int i = 0; i < queue_length(entry->chunks); i++)
	{
		printf("%4d: ", i);
		filetable_print_chunk(queue_get(entry->chunks, i));
	}
}
void filetable_print(FileTable* filetable)
{
	_FileTable* ft = (_FileTable*)filetable;
	
	printf("Printing File Table...\n");
	hashtable_apply(ft->table, (HashTableApplyFunction)filetable_print_file);	
}





























