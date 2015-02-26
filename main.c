#include "common.h"
#ifndef PROTO
#define PROTO
#include "protocol.h"
#endif
#include "socket_utilities.h"


// These structures are used by all the threads to connect to the master server
struct sockaddr_in master_server_addr;

// Timeval structure for slaves
struct timeval tv_struct;

void handle_one_request(struct client_s* client)
{
	// Handle the request for the client.
	ftp_request_t request;
	bzero((void*)&request,sizeof(request));
	char* command;
	char* arg;

	// TODO add the support of open_desc to the existing system calls. Use client structure to store open desc and pass for all the functions
	// accordingly
	// TODO IMPORTANT
	if( read_request(client->client_fd, &request, client->open_desc, client->open_desc_count) == 0 )
	{
		printf("Bad Command\n");
		Write(client_sock, error, strlen(error), client->open_desc, client->open_desc_count);
	}
	// get the command and arg from the structure
	command = request.command;
	arg = request.arg;
}

void thread_function(void* arg)
{
	int thread_no = (int)arg;
	int open_desc[1];
	int open_desc_count = 0;
	// Create structures for the clients
	struct client_s clients[CLIENTS_PER_THREAD];
	int clients_count = 0;

	// Connect to the master thread
	int slave = Socket(AF_INET, SOCK_STREAM, 0, open_desc, open_desc_count);
	open_desc[open_desc_count++] = slave;
	if( Connect( slave, (struct sockaddr*)&master_server_addr, sizeof(master_server_addr), open_desc, open_desc_count) == -1 )
	{
		printf("Can't connect to master server... Exiting\n");
		exit(0);
	}
	// Connected to master server
	// Do fd stuff now
	fd_set fds;
	char buff[BUFF_SIZE];
	while( TRUE )
	{
		FD_ZERO(&fds);
		// Set the slave fd to read in new clients
		FD_SET(slave, &fds) 
		// Set all the existing clients
		// Only control connection fds are set in the fd_set
		add_fds_from_clients_list(&fds,&clients,client_count);
		int res =  select(FD_SETSIZE, &fds, NULL, &fds, tv_struct);
		if( res == -1 )
		{
			//error
		}
		else if( res == 0 )
		{
			// No descriptors are set
		}
		else if( res > 0 )
		{
			// Some descriptors are set.
			// Check if any new clients came
			if( FD_ISSET(slave, &fds) )
			{
				// Can be optimized to take multiple fds at a time
				// New client, so add it to the list
				struct client_s* cli = &clients[client_count];
				cli->client_fd = read_descriptor(slave, open_desc, open_desc_count); 
				cli->file_fd = 0;
				cli->data_fd = 0;
				bzero( (void*)&(cli->act_mode_client_addr), sizeof(cli->act_mode_client_addr) );
				// Send a greeting
				Write(cli->client_fd, greeting, strlen(greeting), open_desc, open_desc_count);
				// Any TODO new files to be cleared here.
				clients_count++;
				FD_CLR(slave, &fds);
			}
		}
		// Now serve the requests and files to the clients
		// Clients not having the data_fd as 0, will be served for the files, otherwise will be served for the requests.
		// Go to each client fd and check if it is set
		for(i=0;i<clients_count;i++)
		{
			if( FD_ISSET(clients[i].client_fd,&fds) )
			{
				// Its set, so serve this client for one request
				
			}
		}
		// Now do the pending file IO
		for(i=0;i<clients_count;i++)
		{
			int fd_cli_file = clients[i].file_fd;
			int fd_cli_data = clients[i].data_fd;
			int fd_cli_control = clients[i].client_fd;
			if( fd_cli_file != 0 && fd_cli_data!=0 && fd_cli_control!=0 )
			{
				int n;
				//File opened, so write one block of data.
				if( ( n = Read(fd_cli_file, buff, BUFF_SIZE, open_desc, open_desc_count)) == 0 )
				{
					// File is completely read, so close all of the descriptors
					close(fd_cli_file);
					close(fd_cli_data);
					close(fd_cli_control);
					// Clean up the entries on the structure, so that this is not used again
					clients[i].file_fd = 0;
					clients[i].client_fd = 0;
					clients[i].data_fd = 0;
				}
				else
				{
					Write(clients[i].data_fd, buff, n, open_desc, open_desc_count);
				}
			}
		}
	}
}


int main()
{
	int open_desc[1];
	int open_desc_count;
	sigignore(SIGPIPE);
	signal(SIGTERM, sig_term_handler);

	// Initialize the timeval struct
	tv_struct.tv_sec = 0;
	tv_struct.tv_usec = 0;	

	// Set resource limits
	set_res_limits();

	// Get the default thread priority
	struct sched_param sp;
	int policy;
	if( pthread_getschedparam(pthread_self(), &policy, &sp) < 0 )
	{
		printf("Error getting the sched params\n");
		exit(0);
	}
	// Check the default sheduling policy.
	if( policy == SCHED_OTHER )
	{
		printf("Its SCHED_OTHER by default and priority is 0\n");
	}
	// Assign the RR scheduler and a high priority
	sp.sched_priority = THREAD_PRIORITY_HIGH;
	if( pthread_setschedparam(pthread_self(), SCHED_RR, &sp) < 0 )
	{
		printf("Error in setting the thread priority in main\n");
	}
	printf("Thread priority set to high and Round Robin in main\n");

	// Change the current working directory to the FILES folder.
	if( chdir("../FTP_FILES") == -1 )
	{
		perror("CWD");
		exit(0);
	}

	// This is the listen socket on 21
	int listen_sock = Socket(AF_INET, SOCK_STREAM, 0, open_desc, open_desc_count);
	struct sockaddr_in server_addr;
	server_addr.sin_port = htons(21);
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htons(INADDR_ANY);
	Bind(listen_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
	Listen(listen_sock, BACKLOG);
	printf("FTP SERVER STARTED\n");


	// thread id
	pthread_t pid;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	// Set stack size to 1 MB
 	if( pthread_attr_setstacksize(&attr,512*1024) < 0 )
	{
		printf("Stack size set error");
		exit(0);
	}
	printf("Stack size set to 512 KB\n");
	// Set the inherit sheduling attr
	if( pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED) < 0 )
	{
		printf("Error in set the inherit sheduling attr from main\n");
	}
	// Now assign the priority and type of scheduling to the attr
	if( pthread_attr_setschedpolicy(&attr, SCHED_RR) < 0 )
	{
		printf("Error in seting the scheduling type\n");
	}
	sp.sched_priority = THREAD_PRIORITY_LOW;
	if( pthread_attr_setschedparam(&attr, &sp) < 0 )
	{
		printf("Error in setting hte priority of the children\n");
	}
	
	/*
	  ALGORITHM
	  1)Set up the TCP server on the main thread for serving the client threads. ( Port 7 , Using port number of echo server )
	  2)Set up the threads ( Pre spawn the threads )
	  3)Each thread then will connect to Localhost, port 7 and get the fd. This fd is then used for Inter thread communication between main and child	      threads. ( Port number is sent as an argument to the child )
	  4)Main thread will Listen and accept connections from the child threads and store the obtained fds in an Array called threads_array, which is th	      en later used for distributing the clients.
	  5)Now the client set up is done.
	  6)Start listening on the port 21.
	  7)Initialize the client count to 0
	  8)If any new client, then see the client_count and then redirect the client to appropriate thread.
	  9)i = client_count / CLIENTS_PER_THREAD
	  10) Write new client fd to threads_array[i];
	*/

	// Step 1:
	// Make a listening socket for the slave
	int master_server = Socket(AF_INET, SOCK_STREAM, 0, open_desc, open_desc_count);
	master_server_addr.sin_port = htons(MASTER_PORT); // Echo port
	master_server_addr.sin_family = AF_INET;
	inet_pton(AF_INET, "127.0.0.1", &(master_server_addr.sin_addr));
	Bind(master_server, (struct sockaddr*)&master_server_addr, sizeof(master_server_addr));
	Listen(master_server, TOTAL_NO_THREADS);
	// Now the server is listening

	// Step 2:
	// Create threads
	int i=0;
	for(;i<TOTAL_NO_THREADS;i++)
	{
		if( pthread_create(&pid, &attr, (void*)thread_function, (void*)i ) != 0 )
        	{
        		printf("pthread create error in main");
        		exit(0);
        	}
	}
	printf("Waiting for slaves to register.......\n");
	// Step 3:
	int slave_fd_array[TOTAL_NO_THREADS];
	// Accept the connetions from the slaves
	for(i=0;i<TOTAL_NO_THREADS;i++)
	{
		int slave_temp  = Accept(master_server, NULL, NULL);
		if( slave_temp < 0 )
		{
			perror("Accept on master_server");
			exit(0);
		}
		slave_fd_array[i] = slave_temp;
	}
	printf("All slaves are registered");
	printf("\nNow, listening on port 21 for clients\n");
	// Step 6

	int clients_count = 0;
	int client_sock;
	while( TRUE )
	{
		client_sock = Accept(listen_sock,NULL, NULL);
		if( client_sock == -1 )
		{
			printf("Error in accepting the client\n");
			continue;
		}
		// Got a client.
		printf("Clients count : %d\n",clients_count);
		int index = clients_count/CLIENTS_PER_THREAD;
		// Send to the FD present on the slave array at this index
		Write(slave_fd_array[index], (char*)&client_sock, FD_SIZE, open_desc, open_desc_count);
		printf("Written a new client:%d to %d thread\n",client_sock,index);
		clients_count++;
	}
}
