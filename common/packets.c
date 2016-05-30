#include "packets.h"
#include <sys/socket.h>

int safe_recv(int socket, void *buffer, size_t length, int flags)
{
	char* cbuf = (char*)buffer;
	
	for (int i = 0; i < length; )
	{
		int ret = recv(socket, cbuf + i, length - i, flags);
		switch (ret)
		{
			case 0:
			{
				return 0;
			}
			case -1:
			{
				return -1;
			}
			default:
			{
				i += ret;
			}
		}
	}
	
	return length;
}
