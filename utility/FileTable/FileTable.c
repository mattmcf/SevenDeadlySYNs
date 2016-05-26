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
	Queue* peers;
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

FileTable* filetable_new()
{
	
}

void filetable_destroy(FileTable* filetable);

// Serializes the provided filesystem to save it to disk or send over the network.
//	filetable  : (not claimed) The filetable to serialize
//	data	   : (not claimed) The serialized data
//	length	   : (not claimed) The length of the serialized data
void filetable_serialize(FileTable* filetable, char** data, int* length);

// Takes some data and deserializes it into a filetable.
//	data      : (not claimed) The data to deserialize
//  bytesRead : (not claimed) The number of bytes read by the deserializer
//	ret	      : (not claimed) The returned filetable
FileTable* filetable_deserialize(char* data, int* bytesRead);

Queue* filetable_get_peers_have_file(FileTable* filetable, char* path);
void   filetable_set_peer_has_file(FileTable* filetable, char* path, int id);
void   filetable_remove_peer(FileTable* filetable, int id);
void   filetable_clear_file(FileTable* filetable, char* path);
