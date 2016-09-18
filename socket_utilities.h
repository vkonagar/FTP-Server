#ifndef UTIL_H
#define UTIL_H
#include "common.h"
#include "protocol.h"

int read_request(int client_sock, ftp_request_t* request, struct client_s* client);

int clean_up_client_structure(struct client_s* client);

/* Invoker of this function will retry on error */
int Connect(int socket, struct sockaddr* addr, int len);

/*
	This returns -1 on the error. Accordingly the invoker of this function should retry or quit
*/
int Socket(int domain, int type, int protocol);

int Bind(int sockfd, const struct sockaddr* myaddr, socklen_t addrlen);

int Listen(int sockfd, int backlog);

int Accept(int sockfd, struct sockaddr* cliaddr, socklen_t* addrlen);

int Read(int clientfd, char* buffer, int size, struct client_s* client);

/*
	Write function: On error, the client's control and data connection are closed. File descriptor is also closed.
*/

int Write(int clientfd, char* buff, int len, struct client_s* client);
#endif
