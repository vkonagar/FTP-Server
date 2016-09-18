#include "protocol.h"

void store_ip_port_active(char* arg,struct sockaddr_in* active_client_addr)
{
	// h1,h2,h3,h4,p1,p2 --> PORT parameter
	// 10,0,0,1,23,42
	char ip[16];
	char p1[5];
	char p2[5];
	int len = strlen(arg);
	int i = 0;
	int count = 0;
	for(;i<len;i++)
	{
		if( arg[i] == ',' )
		{
			if( count == 3 )
			{
				// IP address is parsed
				ip[i++] = '\0';
				break;
			}
			count++;
			ip[i] = '.';
			continue;
		}
		ip[i] = arg[i];
	}
	int flag = 0;
	int p1_count = 0;
	int p2_count = 0;
	// Now parse port
	for( ;i<len;i++ )
	{	
		if( arg[i] == '\0' )
		{
			// End of the arg string
			p1[p1_count] = '\0';
			p2[p2_count] = '\0';
			break;
		}
		else if( arg[i] == ',' )
		{
			flag = 1;
			continue;
		}

		if( flag == 0 )
		{
			p1[p1_count++] = arg[i];
		}
		else if( flag == 1 )
		{
			p2[p2_count++] = arg[i];
		}
	}
	uint16_t port = (atoi(p1)*256)+atoi(p2);
	//printf(" IP : %s Port: %u\n",ip,port);	
	active_client_addr->sin_port = htons(port);
	active_client_addr->sin_family = AF_INET;
	inet_pton(AF_INET, ip, &(active_client_addr->sin_addr));
	return;
}
