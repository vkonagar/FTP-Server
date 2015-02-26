#include "common.h"
#ifndef PROTO
#define PROTO
#include "protocol.h"
#endif

int read_request(int client_sock, ftp_request_t* request, int* open_desc, int open_desc_count)
{
	char* cmd = request->command;
	char* argument = request->arg;
	int cmd_pointer = 0;
	int arg_pointer = 0;
	uint8_t flag = 0;
	char c;
	int err;
	while( ( err = Read(client_sock,&c,1, open_desc, open_desc_count) ) > 0 )
	{
		if( c == ' ' )
		{
			// Command is done, go with the argument
			flag = 1;
			continue;
		}
		else if( c == '\r' )
		{
			if( Read(client_sock,&c,1, open_desc, open_desc_count) == 0 )
			{
				// No \n to read
				printf("\n NO \\n to READ READDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD\n");
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
	printf(" ARGGGGGGGGGG: %s\nCOMMAND:%s\n\n\n",argument,cmd);
	return 0;
}

int Connect(int socket, struct sockaddr* addr, int len, int* arr)
{
	if( connect( socket, addr, len) != 0 )
	{
		perror("Connect at child threads");
		clean_all_fds(arr);
		return -1;
	}
	else
	{
		return 1;
	}
}

int Socket(int domain, int type, int protocol, int* open_desc)
{
	int ret = socket(domain,type,protocol);
	if( ret == -1 )
	{
		perror("Error in creating TCP socket!");
		printf("ERRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRR\n");
		// If there is any other fd to be closed, it is passed her ( FTP control connection, when data connection has the error);
		clean_all_fds(arr);	
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

int Read(int clientfd, char* buffer, int size, int* open_desc, int open_desc_count)
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
			printf("ERRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRR\n");
			perror("Error in reading the line in readLine function : handle_client.h\n");
			// Clean up fd and exit the thread
			clean_all_fds(arr);	
		}
		else if( chars_read == 0 )
		{
			return total_read;
		}
	}
	return size;
}

/*
	This writes the specfied no of bytes in the buffer to the clientfd.
	If the client creashed, then write system call which is called inside this function will return -1, 
	then the client's node is invalidated by setting reachable=0 and will be ultimately deleted by the monitor thread.
*/

int Write(int clientfd, char* buff, int len,  int* open_desc, int open_desc_count)
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
				printf("ERRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRR\n");
				perror("Error with writing\n");
				// Clean up fd and exit the thread
				close(clientfd);
				// If there is any other fd to be closed, it is passed her ( FTP control connection, when data connection has the error);
				int i;
				for(i=0;i<open_desc_count;i++)
				{
					close(open_desc[i]);
				}
				decrement_thread_count();
				pthread_exit(0);
			}
		}
		written_chars += temp;
		left_chars -= left_chars;
	}
}
