#ifndef PROTOCOL_H
#define PROTOCOL_H
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


void store_ip_port_active(char* arg,struct sockaddr_in* active_client_addr);
#endif
