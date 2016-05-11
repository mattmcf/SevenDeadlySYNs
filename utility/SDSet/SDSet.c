#include "SDSet.h"
#include <stdlib.h>
#include <stdio.h>

#define INITITIAL_SIZE (10)

typedef struct
{
	unsigned int* dense;
	unsigned int* sparse;
	void** values;
	int size;
	int space;
} _SDSet;

SDSet* sdset_new()
{
	_SDSet* set = (_SDSet*)malloc(sizeof(_SDSet));
	
	set->dense = (unsigned int*)malloc(INITITIAL_SIZE * sizeof(unsigned int));
	set->sparse = (unsigned int*)malloc(INITITIAL_SIZE * sizeof(unsigned int));
	set->values = (void**)malloc(INITITIAL_SIZE * sizeof(void*));
	
	set->size = 0;
	set->space = INITITIAL_SIZE;
	
	return (SDSet*)set;
}
void sdset_destroy(SDSet* set)
{
	_SDSet* s = (_SDSet*)set;
	free(s->dense);
	free(s->sparse);
	free(s->values);
	free(s);
}
void sdset_clear(SDSet* set)
{
	_SDSet* s = (_SDSet*)set;
	s->size = 0;
}
int sdset_is_set(SDSet* set, int key)
{
	_SDSet* s = (_SDSet*)set;
		
	return (s->sparse[key] < s->size && s->dense[s->sparse[key]] == key);
}
void sdset_set(SDSet* set, int key, void* value)
{
	_SDSet* s = (_SDSet*)set;
	
	while (key >= s->space)
	{
		s->space *= 2;
		s->dense = (unsigned int*)realloc(s->dense, s->space * sizeof(unsigned int));
		s->sparse = (unsigned int*)realloc(s->sparse, s->space * sizeof(unsigned int));
		s->values = (void**)realloc(s->values, s->space * sizeof(void*));
	}
		
    s->dense[s->size] = key;
    s->sparse[key] = s->size;
	s->values[key] = value;
    s->size++;
}
void* sdset_get(SDSet* set, int key)
{
	_SDSet* s = (_SDSet*)set;
	
	if (sdset_is_set(set, key))
	{
		return s->values[key];
	}
	return NULL;
}
int sdset_length(SDSet* set)
{
	_SDSet* s = (_SDSet*)set;
	return s->size;
}
void* sdset_get_index(SDSet* set, int index)
{
	_SDSet* s = (_SDSet*)set;
	if (index >= s->size)
	{
		return NULL;
	}
	
	return s->values[s->dense[index]];
}




