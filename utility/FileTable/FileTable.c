#include <stdlib.h>
#include <string.h>
#include "FileTable.h"
#include "../HashTable/HashTable.h"
#include "../Queue/Queue.h"
#include "../ChunkyFile/ChunkyFile.h"
#include <assert.h>
#include <stdio.h>
#include "../ColoredPrint/ColoredPrint.h"

int PRINT_FMT = -1;
int ERR_FMT = -1;
int PATH_FMT = -1;

typedef struct
{
	HashTable* table;
} _FileTable;

void filetableentry_destroy(FileTableEntry* fte)
{
	queue_apply(fte->chunks, (QueueApplyFunction)queue_destroy);
	queue_destroy(fte->chunks);
	queue_destroy(fte->work_queue);
	queue_destroy(fte->outstanding_requests);
	free(fte->path);
	free(fte);
	if (fte->file)
	{
		chunkyfile_destroy(fte->file);
	}
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
	if (ERR_FMT == -1)
	{
		FORMAT_ARG err_arg[] = {COLOR_L_RED, COLOR_BOLD, COLOR_UNDERLINE, 0};
		ERR_FMT = register_format(err_arg);
		FORMAT_ARG print_arg[] = {COLOR_L_BLUE, COLOR_BOLD, 0};
		PRINT_FMT = register_format(print_arg);
		FORMAT_ARG path_arg[] = {COLOR_BOLD, COLOR_UNDERLINE, 0};
		PATH_FMT = register_format(path_arg);
	}
	
	_FileTable* ft = (_FileTable*)malloc(sizeof(_FileTable*));
	ft->table = hashtable_new((HashFunction)filetableentry_hash, (ElementEqualsFunction)filetableentry_equals);
	return (FileTable*)ft;
}

void filetable_destroy(FileTable* filetable)
{
	_FileTable* ft = (_FileTable*)filetable;
	hashtable_apply(ft->table, (HashTableApplyFunction)filetableentry_destroy);
	hashtable_destroy(ft->table);
	free(ft);
}

void filetable_enqueue_work_request(FileTable* filetable, char* path, int chunk)
{
	_FileTable* ft = (_FileTable*)filetable;
	
	FileTableEntry search;
	search.path = path;
	FileTableEntry* fte = hashtable_get_element(ft->table, &search);
	
	queue_push(fte->work_queue, (void*)(long)chunk);
}

// returns 1
//	if outstanding request < max pending request & there are pending requests
int filetableentry_get_job(FileTableEntry* entry, int max_pending_reqests, int* chunk, int* peer, long* job_id)
{
	static long JOB_ID = 0;
	
	if (queue_length(entry->outstanding_requests) < max_pending_reqests && queue_length(entry->work_queue) > 0)
	{
		*chunk = (int)(long)queue_pop(entry->work_queue);
		*job_id = JOB_ID++;
		
		Queue* peers_with_chunk = queue_get(entry->chunks, *chunk);
		int num_peers_with_chunk = queue_length(peers_with_chunk);
		assert(num_peers_with_chunk);
		*peer = (int)(long)queue_get(peers_with_chunk, rand() % num_peers_with_chunk);
		queue_push(entry->outstanding_requests, (void*)(long)(*job_id));
		
		return 1;
	}
	return 0;
}

void filetable_add_filesystem(FileTable* filetable, FileSystem* filesystem, int peer, int needs_data)
{
	_FileTable* ft = (_FileTable*)filetable;
	FileSystemIterator* fsi = filesystemiterator_relative_new(filesystem, 1);
	
	FileTableEntry fte;
	int length;
	time_t mod_time;
	while ((fte.path = filesystemiterator_next(fsi, &length, &mod_time)))
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
		newEntry->work_queue = queue_new();
		newEntry->outstanding_requests = queue_new();
		
		if (needs_data)
		{
			for (int i = 0; i < numChunks; i++)
			{
				queue_push(newEntry->work_queue, (void*)(long)i);
			}
			queue_shuffle(newEntry->work_queue);
		}
		
		newEntry->file = NULL;
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
	FileSystemIterator* fsi = filesystemiterator_relative_new(filesystem, 1);
	
	FileTableEntry fte;
	int length;
	time_t mod_time;
	while ((fte.path = filesystemiterator_next(fsi, &length, &mod_time)))
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

#define START_TABLE  (0xFF)
#define START_ENTRY  (0xFE)
#define START_CHUNK  (0xFC)
#define START_ID     (0xFB)
#define END_CHUNK    (0xFA)
#define END_ENTRY    (0xF8)
#define END_TABLE	 (0xF7)

void filetable_serialize(FileTable* filetable, char** data, int* length)
{
	_FileTable* ft = (_FileTable*)filetable;
	Queue* ser = queue_new();
	
	queue_push(ser, (void*)(long)START_TABLE);
		
	HashTableIterator* hti = hashtableiterator_new(ft->table);
	if (hti)
	{
		FileTableEntry* entry;
		while ((entry = hashtableiterator_next(hti)))
		{
			queue_push(ser, (void*)(long)START_ENTRY);
			push_string(ser, entry->path);
			
			for (int i = 0; i < queue_length(entry->chunks); i++)
			{
				queue_push(ser, (void*)(long)START_CHUNK);
				Queue* chunk = queue_get(entry->chunks, i);
				
				for (int j = 0; j < queue_length(chunk); j++)
				{
					queue_push(ser, (void*)(long)START_ID);
					int peer = (int)(long)queue_get(chunk, j);
					char buf[50];
					sprintf(buf, "%d", peer);
					push_string(ser, buf);
				}
				queue_push(ser, (void*)(long)END_CHUNK);
			}
			queue_push(ser, (void*)(long)END_ENTRY);
		}
	}
	
	queue_push(ser, (void*)(long)(END_TABLE));
	
	*length = queue_length(ser);
	*data = (char*)malloc(*length * sizeof(char));
	for (int i = 0; i < *length; i++)
	{
		(*data)[i] = (char)(long)queue_get(ser, i);
	}
	queue_destroy(ser);
}

FileTable* filetable_deserialize(char* data, int* bytesRead)
{
	_FileTable* deser = NULL;
	FileTableEntry* entry = NULL;
	Queue* chunk = NULL;
	
	int i = 0;
	
	assert((unsigned char)data[0] == START_TABLE);
	
	while (1)
	{
		switch ((unsigned char)data[i])
		{
			case START_TABLE:
			{
				deser = (_FileTable*)filetable_new();
				i += 1;
				break;
			}
			case START_ENTRY:
			{
				entry = (FileTableEntry*)malloc(sizeof(FileTableEntry));
				entry->chunks = queue_new();
				entry->work_queue = queue_new();
				entry->outstanding_requests = queue_new();
				entry->file = NULL;
				i += 1;
				entry->path = copy_string(data + i);
				i += strlen(entry->path) + 1;
				break;
			}
			case START_CHUNK:
			{
				chunk = queue_new();
				i += 1;
				break;
			}
			case START_ID:
			{
				i += 1;
				int numRead;
				int peer;
				sscanf(data + i, "%d%n", &peer, &numRead);
				queue_push(chunk, (void*)(long)peer);
				i += numRead + 1;
				break;
			}
			case END_CHUNK:
			{
				queue_push(entry->chunks, chunk);
				i += 1;
				break;
			}
			case END_ENTRY:
			{
				hashtable_add(deser->table, entry);
				i += 1;
				break;
			}
			case END_TABLE:
			{
				i += 1;
				*bytesRead = i;
				return (FileTable*)deser;
			}
			default:
			{
				format_printf(ERR_FMT, "Filetable deserialization error.\n");
				assert(0);
			}
		}
	}
	return NULL;
}

int filetable_get_num_chunks(FileTable* filetable, char* path)
{
	_FileTable* ft = (_FileTable*)filetable;
	
	FileTableEntry search;
	search.path = path;
	FileTableEntry* fte = hashtable_get_element(ft->table, &search);
	
	return queue_length(fte->chunks);
}

Queue* filetable_get_peers_who_have_file_chunk(FileTable* filetable, char* path, int chunk)
{
	_FileTable* ft = (_FileTable*)filetable;
	
	FileTableEntry search;
	search.path = path;
	FileTableEntry* fte = hashtable_get_element(ft->table, &search);
	
	if (fte && queue_length(fte->chunks) >= chunk)
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
		filetable_print(filetable);
		format_printf(ERR_FMT, "Trying to access chunk %d of file (%s) that does not exist\n", chunkNum, path);
		assert(0);
	}
	
	for (int i = 0; i < queue_length(chunk); i++)
	{
		if ((int)(long)queue_get(chunk, i) == peer)
		{
			format_printf(ERR_FMT, "Trying to add peer %d to chunk %d that peer already has\n", peer, chunkNum);
			//assert(0);
			return;
		}
	}
	queue_push(chunk, (void*)(long)peer);
}

int remove_peer(void* elementp, void* userData)
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
		printf("%s\n", fte->path);
		for (int i = 0; i < queue_length(fte->chunks); i++)
		{
			Queue* old = queue_get(fte->chunks, i);
			Queue* filtered = queue_filter(old, (QueueFilterFunction)remove_peer, (void*)(long)id);
			queue_set(fte->chunks, filtered, i);			
			queue_destroy(old);
		}
	}
	hashtableiterator_destroy(hti);
}

void filetable_remove_file(FileTable* filetable, char* path)
{
	_FileTable* ft = (_FileTable*)filetable;
	
	FileTableEntry search;
	search.path = path;
	FileTableEntry* removed = hashtable_remove_element(ft->table, &search);
	if (removed)
	{
		filetableentry_destroy(removed);		
	}
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
	format_printf(PATH_FMT, "%s\n", entry->path);	
	for (int i = 0; i < queue_length(entry->chunks); i++)
	{
		printf("%4d: ", i);
		filetable_print_chunk(queue_get(entry->chunks, i));
	}
}
void filetable_print(FileTable* filetable)
{
	/*
	_FileTable* ft = (_FileTable*)filetable;
	
	format_printf(PRINT_FMT, "Printing File Table...\n");
	hashtable_apply(ft->table, (HashTableApplyFunction)filetable_print_file);	*/
}

ChunkyFile* filetable_get_chunkyfile(FileTable* filetable, char* path)
{
	_FileTable* ft = (_FileTable*)filetable;
	
	FileTableEntry search;
	search.path = path;
	FileTableEntry* fte = hashtable_get_element(ft->table, &search);
	
	if (fte)
	{
		return fte->file;
	}
	return NULL;
}

void filetable_set_chunkyfile(FileTable* filetable, char* path, ChunkyFile* file)
{
	_FileTable* ft = (_FileTable*)filetable;
	
	FileTableEntry search;
	search.path = path;
	FileTableEntry* fte = hashtable_get_element(ft->table, &search);
	
	if (fte)
	{
		fte->file = file;
	}
}

/* ------------------ ITERATOR ------------------ */


FileTableIterator* filetableiterator_new(FileTable* filetable)
{
	_FileTable * ft = (_FileTable*)filetable;

	return (FileTableIterator*)hashtableiterator_new(ft->table);
}

// returns path
FileTableEntry* filetableiterator_path_next(FileTableIterator* iterator)
{
	return hashtableiterator_next((HashTableIterator*)iterator);
}

void filetableiterator_destroy(FileTableIterator* iterator) 
{
	hashtableiterator_destroy((HashTableIterator*)iterator);
}

























