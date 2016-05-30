#include "packets.h"

int safe_recv(int socket, void *buffer, size_t length, int flags)
{
	char* cbuf = (char*)buffer;
	
	for (int i = 0; i < length; i++)
	{
		switch (recv(socket, cbuf + i; length - i; flags))
		{
			case 0:
			{
				return 0;
			}
			case -1;
			{
				return -1;
			}
		}
	}
	
	return length;
}