#include <stdlib.h>
#include <string.h>
#include "FileTable.h"
#include "../HashTable/HashTable.h"
#include "../Queue/Queue.h"

typedef struct
{
	HashTable* table;
} _FileTable;

typedef struct
{
	char* path;
	Queue* chunks;
} FileTableEntry;

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

void filetable_serialize(FileTable* filetable, char** data, int* length);

FileTable* filetable_deserialize(char* data, int* bytesRead);

Queue* filetable_get_peers_have_file(FileTable* filetable, char* path);
void   filetable_set_peer_has_file(FileTable* filetable, char* path, int id);
void   filetable_remove_peer(FileTable* filetable, int id);
void   filetable_clear_file(FileTable* filetable, char* path);
