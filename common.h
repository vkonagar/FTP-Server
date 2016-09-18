#ifndef COMMON_H
#define COMMON_H

// General
#include <limits.h>
#include <sys/resource.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <pthread.h>
#include <assert.h>
#include <sched.h>

// Networking
#include <sys/epoll.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
// ############# Server parameters #############

#define CLIENTS_PER_THREAD 10
#define TOTAL_NO_THREADS 1000
#define MAX_EVENTS (2*CLIENTS_PER_THREAD+1)
#define NO_OF_CORES 24

// #############################################

// Macros
#define FD_SIZE sizeof(int)
#define MASTER_PORT 7
#define TRUE 1
#define FALSE 0
#define BACKLOG 100000
#define THREAD_PRIORITY_LOW 20
#define THREAD_PRIORITY_HIGH 50
#define REQ_COMMAND_LENGTH 20
#define REQ_ARG_LENGTH 40
#define BUFF_SIZE 1024
#define FILE_NAME_LEN 20

int clients_active = 0;
pthread_mutex_t mutex;

// Structs
typedef struct arguments
{
	int cli_sock;
}arguments_t;


struct client_s
{
	int client_fd;
	int data_fd;
	int file_fd;
	struct sockaddr_in act_mode_client_addr;
};


void increment_clients_count();

void decrement_clients_count();

void set_res_limits();

int read_descriptor(int slave, struct client_s* client);

void sig_term_handler();

void sig_pipe_handler();
#endif
