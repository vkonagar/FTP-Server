#include "common.h"

/*
	This header file contains all the protocol header required for the FTP
*/

char* greeting = "220 Hello\012";

char* system_str = "215 Ubuntu 10.04\012";

char* port_reply = "200 Got port and IP\012";

char* allow_user = "230 Hey\012";

char* file_error = "451 File open error\012";

char* file_ok = "150 File ok. Transferring\012";

char* file_done = "226 File transferred\012";

char* type_ok = "200 Type OK\012";

char* error = "451 Bad command\012";

char* close_con = "221 close\012";

char* data_open_error = "425 Can't open data connection\012";

typedef struct ftp_request
{
	char command[REQ_COMMAND_LENGTH];
	char arg[REQ_ARG_LENGTH]; 
}ftp_request_t;


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
