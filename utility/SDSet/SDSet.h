#ifndef SDSET_H
#define SDSET_H

typedef struct SDSet SDSet;

SDSet* sdset_new();
void sdset_clear(SDSet* set);
void sdset_destroy(SDSet* set);

void sdset_set(SDSet* set, int key, void* value);
int sdset_is_set(SDSet* set, int key);
void* sdset_get(SDSet* set, int key);

int sdset_length(SDSet* set);
void* sdset_get_index(SDSet* set, int index);

#endif