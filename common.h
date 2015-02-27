#ifndef COMMON_HEADERS
#define COMMON_HEADERS

// General
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
// Networking
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
// ############# Server parameters #############

#define CLIENTS_PER_THREAD 10
#define TOTAL_NO_THREADS 1

// #############################################

// Macros
#define FD_SIZE sizeof(int)
#define MASTER_PORT 7
#define TRUE 1
#define FALSE 0
#define BACKLOG 100000
#define MAX_OPEN_DESC 1024
#define THREAD_PRIORITY_LOW 20
#define THREAD_PRIORITY_HIGH 50
#define REQ_COMMAND_LENGTH 20
#define REQ_ARG_LENGTH 40
#define BUFF_SIZE 1024
#define FILE_NAME_LEN 20
#define MAX_OPEN_DESC_CLIENT 4

int threads_active = 0;
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


void increment_thread_count()
{
        // Lock
        pthread_mutex_lock(&mutex);
        threads_active++;
        // Unlock
        pthread_mutex_unlock(&mutex);
}

void decrement_thread_count()
{
        // Lock
        pthread_mutex_lock(&mutex);
        threads_active--;
        // Unlock
        pthread_mutex_unlock(&mutex);
}


void set_res_limits()
{
	// Set MAX FD's to 50000
	struct rlimit res;
	res.rlim_cur = 50000;
	res.rlim_max = 50000;
	if( setrlimit(RLIMIT_NOFILE, &res) == -1 )
	{
		perror("Resource FD limit");
		exit(0);
	}
	printf("FD limit set to 50000\n");	
	if( setrlimit(RLIMIT_RTPRIO, &res) == -1 )
	{
		perror("Resource Prioiry limit");
		exit(0);
	}
	printf("Prioirty limit set to 100\n");
}


void clean_all_fds(int fd[],int count)
{
	int i;
	for(i=0;i<count;i++)
	{
		close(fd[i]);
	}
}

int monitoring_thread()
{
	while(1)
	{
		sleep(5);
		printf("Current threads: %d\n",threads_active);
	}
}


int read_descriptor(int slave, struct client_s* client)
{
	int client_fd;
	int n = Read(slave,(char*)&client_fd,FD_SIZE,client);
	if( n == FD_SIZE )
	{
		return client_fd;
	}
	return -1;
}


void sig_term_handler()
{
	// Kill all threads
	printf("FTP Server exiting\n");
	exit(0);
}

void sig_pipe_handler()
{
	printf("Client terminated because of Pipe termination\n");
}
#endif
