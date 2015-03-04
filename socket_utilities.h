#include "common.h"
#ifndef PROTO
#define PROTO
#include "protocol.h"
#endif

int read_request(int client_sock, ftp_request_t* request, struct client_s* client)
{
	char* cmd = request->command;
	char* argument = request->arg;
	int cmd_pointer = 0;
	int arg_pointer = 0;
	uint8_t flag = 0;
	char c;
	int err;
	while( ( err = Read(client_sock,&c,1, client) ) > 0 )
	{
		if( c == ' ' )
		{
			// Command is done, go with the argument
			flag = 1;
			continue;
		}
		else if( c == '\r' )
		{
			if( Read(client_sock,&c,1, client) == 0 )
			{
				// No \n to read
				cmd[cmd_pointer] = '\0';
                                argument[arg_pointer] = '\0';
                                return 1;
			}
			else if( c == '\n' )
			{
				cmd[cmd_pointer] = '\0';
				argument[arg_pointer] = '\0';
				return 1;
			}
			else
			{
				break;
			}
		}

		if( flag == 0 )
		{
			// COMMAND
			cmd[cmd_pointer++] = c;
		}
		else if( flag == 1 )
		{
			// ARGUMENT
			argument[arg_pointer++] = c;
		}
	}
	return 0;
}

int clean_up_client_structure(struct client_s* client)
{
	if( client->client_fd != 0 )
	{
		Write(client->client_fd, close_con, strlen(close_con), client);
		//close(client->client_fd);
	}
	if( client->data_fd != 0 )
	{
		//close(client->data_fd);
	}
	if( client->file_fd != 0 )
	{
		//close(client->file_fd);
	}
	//client->client_fd = 0;
	//client->data_fd = 0;
	//client->file_fd = 0;
}


/* Invoker of this function will retry on error */
int Connect(int socket, struct sockaddr* addr, int len)
{
	if( connect( socket, addr, len) != 0 )
	{
		perror("Connect at child threads");
		return -1;
	}
	else
	{
		return 1;
	}
}

/*
	This returns -1 on the error. Accordingly the invoker of this function should retry or quit
*/

int Socket(int domain, int type, int protocol)
{
	int ret = socket(domain,type,protocol);
	if( ret == -1 )
	{
		perror("Error in creating TCP socket!");
		// If there is any other fd to be closed, it is passed her ( FTP control connection, when data connection has the error);
	}
	return ret;
}

int Bind(int sockfd, const struct sockaddr* myaddr, socklen_t addrlen)
{
	int ret = bind(sockfd, myaddr, addrlen);
	if( ret == -1 )
	{
		perror("Error in binding the address at the server");
		exit(0);
	}
	return ret;
}

int Listen(int sockfd, int backlog)
{
	int ret;
	ret = listen(sockfd, backlog);
	if( ret == -1 )
	{
		perror("TCP server could not listen\n");
		exit(0);
	}
	return ret;
}

int Accept(int sockfd, struct sockaddr* cliaddr, socklen_t* addrlen)
{
	int ret;
	ret = accept(sockfd, cliaddr, addrlen);
	if( ret == -1 )
	{
		perror("TCP server cannot accept incoming clients \n");
	}
	return ret;
}

int Read(int clientfd, char* buffer, int size, struct client_s* client)
{
	int char_count = size;
	int chars_read = 0;
	int total_read = 0;
	while( char_count > 0 )
	{
		if( ( chars_read = read(clientfd, buffer + chars_read , char_count ) ) > 0 )
		{
			char_count = char_count - chars_read;
			total_read = total_read + chars_read;
			if( char_count == 0 )
			{
				// All chars are read, break out
				break;
			}
		}
		if( chars_read == -1 )
		{
			if( errno == EINTR)
				continue;
			perror("Error in reading the line in readLine function : handle_client.h\n");
			if( client!= NULL )
			{
				clean_up_client_structure(client);
			}
			return -1;
		}
		else if( chars_read == 0 )
		{
			return total_read;
		}
	}
	return size;
}

/*
	Write function: On error, the client's control and data connection are closed. File descriptor is also closed.
*/

int Write(int clientfd, char* buff, int len, struct client_s* client)
{
	int left_chars = len;
	int written_chars = 0;
	int temp;

	// This will write untill the no of characters specified have been written
	while( left_chars > 0 )
	{
		if( (temp = write(clientfd, buff+written_chars, left_chars)) <= 0 )
		{
			// Error
			if( temp < 0 && errno == EINTR)
			{
				continue;
			}
			else
			{
				perror("Error with writing EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEeee\n");
				if( client!= NULL )
				{
					clean_up_client_structure(client);
				}
				return -1;
			}
		}
		written_chars += temp;
		left_chars -= left_chars;
	}
}
