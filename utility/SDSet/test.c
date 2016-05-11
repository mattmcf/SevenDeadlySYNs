#include "SDSet.h"
#include <stdio.h>

int main()
{
	SDSet* set = sdset_new();
	
	sdset_set(set, 4, "abc");
	sdset_set(set, 20, "def");
	sdset_set(set, 0, "ghi");
		
	for (int a = 0; a < 2; a++)
	{
		for (int i = 0; i < 30; i++)
		{
			if (sdset_is_set(set, i))
			{
				printf("%d is set: %s\n", i, (char*)sdset_get(set, i));
			}
			else
			{
				printf("%d is not set\n", i);
			}
		}
	
		for (int i = 0; i < sdset_length(set); i++)
		{
			printf("%s\n", sdset_get_index(set, i));
		}
	
		sdset_clear(set);
	}
	
	sdset_destroy(set);
	return 0;
}