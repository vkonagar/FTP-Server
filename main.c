#include "common.h"
#include "protocol.h"
#include "socket_utilities.h"

// These structures are used by all the threads to connect to the master server
struct sockaddr_in master_server_addr;

int num_cores=0;

int stick_this_thread_to_core(int core_id) {
	if (core_id < 0 || core_id >= num_cores)
		return EINVAL;

   	cpu_set_t cpuset;
   	CPU_ZERO(&cpuset);
   	CPU_SET(core_id, &cpuset);

  	 pthread_t current_thread = pthread_self();    
   	return pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
}

void monitor()
{
	while(1)
	{
		printf("Count : %d\n",clients_active);
		sleep(2);
	}
}

// connected clients count for each thread
int thread_clients_count[TOTAL_NO_THREADS];


void set_sched_param()
{

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
}

/*
	-1 is returned when the request format is wrong
	1 is returned when the request format is correct
*/
int handle_one_request(struct client_s* client, int epfd, int client_count,struct epoll_event* one_event)
{
	

	int ret;
	// Handle the request for the client.
	ftp_request_t request;
	bzero((void*)&request,sizeof(request));
	char* command;
	char* arg;
	int client_sock = client->client_fd;
	int read_code = read_request(client_sock, &request, client);
	if( read_code == 0 )
	{
		// Bad request format
		printf("Bad request format\n");
		Write(client_sock, error, strlen(error), client);
		// Close and clean up
		return -1;
	}
	// get the command and arg from the structure
	command = request.command;
	arg = request.arg;
	// Check if the file is being served by the server to the client. then , don't accept any commands
	// printf("Command %s\nARg:%s\n",command,arg);
	if( strcmp(command,"USER") == 0 )
	{
		// USER REQUEST
		Write(client_sock, allow_user, strlen(allow_user), client);
	}
	else if( strcmp(command,"SYST") == 0 )
	{
		// SYSTEM REQUEST
		Write(client_sock, system_str, strlen(system_str), client);
	}
	else if( strcmp(command,"PORT") == 0 )
	{
		// PORT COMMAND
		// Send reply
		Write(client_sock, port_reply, strlen(port_reply), client);
		// Argument is of form h1,h2,h3,h4,p1,p2
		store_ip_port_active(arg,&(client->act_mode_client_addr));
	}
	else if( strcmp(command,"TYPE") == 0 )
	{
		Write(client_sock, type_ok, strlen(type_ok), client);
	}
	else if( strcmp(command,"QUIT") == 0 )
	{
		// Child exited
		Write(client_sock, close_con, strlen(close_con), client);
		// break and free the client_sock
		printf("MSG: Client sent QUIT\n");
		return -1;
	}
	else if( (strcmp(command,"LIST") == 0 ) || (strcmp(command,"RETR") == 0 ))
	{
		// RETR
		// Argument will have the file name
		int file;
		if( strcmp(command,"LIST") == 0)
		{
			// Execute the list command and store it in a file
			system("ls -l > .list");
			// Now open that file and send it to the client
			file = open(".list",O_RDONLY);
		}
		else
		{
			file = open(arg,O_RDONLY);
		}
		if( file == -1 )
		{
			perror("Open");
			Write(client_sock, file_error, strlen(file_error), client);
			// If the file open has error, quit the connection.
			printf("MSG: File open error\n");
			return -1;
		}
		// FILE OK
		// Store the file fd in the struct
		client->file_fd = file;
		// Write File OK
		Write(client_sock, file_ok, strlen(file_ok), client);
		// Now transfer the file to the client
		int data_sock = Socket(AF_INET, SOCK_STREAM, 0);
		if( data_sock == -1 )
		{
			perror("Socket");
			printf("MSG: SOcket creation error\n");
			return -1;
		}	
		sleep(5);
		if( connect(data_sock, (struct sockaddr*)&(client->act_mode_client_addr), sizeof(client->act_mode_client_addr)) == -1 )
		{
			char ip_dst[15];
			inet_ntop(AF_INET, &(client->act_mode_client_addr.sin_addr), ip_dst, 15);
			printf("MSG: Cant establish data connection to %s:%d -- ",ip_dst, ntohs(client->act_mode_client_addr.sin_port));
			perror("Connect error");
			// Close existing fd's related to this command
			Write(client_sock, data_open_error, strlen(data_open_error), client);
			return -1;
		}
		else
		{
			// Connection established
			// printf("Connection established from server to client\n");
			client->data_fd = data_sock;
			// Set the event
			
			// Add this client data con to the epollfd.
			one_event->data.fd = client_count; /* return the data to us later */
			one_event->events = EPOLLOUT;
			
			// Set this fd for writing
			ret = epoll_ctl(epfd, EPOLL_CTL_ADD, data_sock, one_event);
			if (ret)
			{
				perror ("MSG: epoll_ctl");
				printf("MSG: ERROR iN ADDING THE DATA FD\n");
				return -1;
			}
			// Now dont look for an event of the client fd until this file is tranferred.
			ret = epoll_ctl (epfd, EPOLL_CTL_DEL, client_sock, one_event);
			if (ret)
			{
				perror ("MSG: epoll_ctl");
				printf("MSG: ERROR iN ADDING THE DATA FD\n");
				return -1;
			}
		//	printf("Client added\n");
		}
	}
	else
	{
		Write( client_sock, error, strlen(error), client);
		printf("MSG: unrecognized command %s\n",command);
		return -1;
	}
	return 1;
}

void thread_function(void* arg)
{	
	// One event is used as a placeholder to add a new event
	struct epoll_event one_event;
	
	// Thread no given by main thread
	int thread_no = (int)arg;

	/*
	 * Assign the thread to a core ( affinity )
	 */

	int core = thread_no % num_cores;

	printf("Thread: %d affinity to core:%d\n",thread_no, core);

	if( stick_this_thread_to_core(core) != 0 )
	{
			/*
			 * Error in setting the affinity
			 */
		printf("Can't set the affinity\n");
		exit(0);
	}


	// Create structures for the clients
	struct client_s clients[CLIENTS_PER_THREAD];
	
	// This is the count for the clients of this thread
	int* clients_count = &thread_clients_count[thread_no];

	// Connect to the master thread
	int slave = Socket(AF_INET, SOCK_STREAM, 0);
	if( Connect( slave, (struct sockaddr*)&master_server_addr, sizeof(master_server_addr) ) == -1 )
	{
		printf("Can't connect to master server... Exiting\n");
		close(slave);
		pthread_exit(0);
	}
	// Connected to master server
	printf("Connected to master\n");
	
	// Serve the clients now!!!!!!
	int i,ret,nr_events;
	char buff[BUFF_SIZE];
	// Create an epoll fd, should be closed with close system call
	int epfd = epoll_create(MAX_EVENTS);
	if( epfd < 0 )
	{
		perror("ERROR IN EPOLL IN A THREAD");
		close(slave);
		pthread_exit(0);
	}

	// Create epoll events array. We need at max CLIENTS_PER_THREAD for client_control fds, CLIENTS_PER_THREAD for client
	// data fds and 1 for slave fd, in the case where all are active.
	// All the returned events are stored here.
	struct epoll_event *all_events;
	all_events = (struct epoll_event*) calloc (MAX_EVENTS, sizeof (struct epoll_event));
	if (!all_events)
	{
	        perror ("malloc");
		// Free and close all fds of this thread.
		free(all_events);
		close(epfd);
		close(slave);
		pthread_exit(0);
	}
	/* add the slave to the epollfd for reading */
	one_event.events = EPOLLIN;

	// Give -1 as slave id
	one_event.data.fd = -1;
	// Set this fd for reading
	ret = epoll_ctl(epfd, EPOLL_CTL_ADD, slave, &one_event);
	if (ret)
	{
		perror ("epoll_ctl");
		free(all_events);
		close(epfd);
		close(slave);
		pthread_exit(0);
	}
	/* 
		Event loop
	*/
	while( TRUE )
	{
		/*
			epoll system call with Level triggered mode enabled 
			Add all the client_fds, data_fds, and slave
			-1 as timeout will block untill some of them are ready.
		*/
		nr_events = epoll_wait(epfd, all_events, MAX_EVENTS, -1);
		if (nr_events < 0)
		{
		        perror ("epoll_wait");
			continue;
		}
		// poll_wait returned correctly
		// Now check the events
		for( i=0; i<nr_events; i++)
		{
			// Check for errors
			if( (all_events[i].events & EPOLLERR) || (all_events[i].events & EPOLLHUP) )
			{
				// Error in this fd
				printf("ERROR IN THE FD\n");
				// Check if it is slave, then kill the thread.
				if( all_events[i].data.fd == -1 )
				{
					printf("ERROR : Error in the master socket\n");
					close(slave);
					// If there are no clients, then kill this thread
					if( *clients_count == 0 )
					{
						close(epfd);
						free(all_events);
						pthread_exit(0);
					}
				}
				else
				{
					// Its a client fd
					int temp_cid = (all_events[i].data.fd);
					clean_up_client_structure(&clients[temp_cid]);
				}
				continue;
			}
			
			// Check if master has got something
			// -1 is for slave
			if( all_events[i].data.fd == -1 )
			{
				// printf("I AM A SLAVE\n");
				// Master has got something.
				// printf("Master has got something for me\n");
				struct client_s* cli = &clients[*clients_count];
				int read_desc = read_descriptor(slave,cli); 
				cli->client_fd = read_desc;
				cli->file_fd = 0;
				cli->data_fd = 0;
				bzero( (void*)&(cli->act_mode_client_addr), sizeof(cli->act_mode_client_addr) );
				
				// Add this client to the epollfd.
				// Negate the descriptor and store, so that it won't clash with the slave desc
				one_event.data.fd = (*clients_count); /* return the data to us later */
				one_event.events = EPOLLIN;
				// Set this fd for reading
				ret = epoll_ctl (epfd, EPOLL_CTL_ADD, cli->client_fd, &one_event);
				if (ret)
				{
					perror ("epoll_ctl");
					printf("Unable to add fd to events\n");
					clean_up_client_structure(cli);
					continue;
				}
				// Everything is perfect now
				// Send a greeting
				Write(cli->client_fd, greeting, strlen(greeting), cli);	
				// Clients increased
				(*clients_count)++;
				// printf("THREAD %d Clients count : %d \n",thread_no,*clients_count);
				// Go to the next iteration.
				continue;
			}

			// This current event is by client fd not master.
			/*
				Convert the stored negative desc to positive on the event structure back to an integer for referencing the client
			*/
			int cur_cid = (all_events[i].data.fd);
			// Check if clients control fds are ready for reading
			// if this fd is not slave and is for reading, then its client control connection fd
			if( all_events[i].events & EPOLLIN )
			{
				// Serve the request.
				if( handle_one_request(&clients[cur_cid], epfd, cur_cid, &one_event) == -1 )
				{
					printf("ERROR: Some error in the command\n");
					// Some error in the commands. Quit the connection
					clean_up_client_structure(&clients[cur_cid]);
				}
			}
			else if( all_events[i].events & EPOLLOUT )
			{
				// Serve the FILE
				int fd_cli_file = clients[cur_cid].file_fd;
			        int fd_cli_data = clients[cur_cid].data_fd;
				int fd_cli_control = clients[cur_cid].client_fd;
				int read_n;
				//File opened, so write one block of data.
				if( ( read_n = Read(fd_cli_file, buff, BUFF_SIZE, &clients[cur_cid])) == 0 )
				{
					// File is completely read, so close all of the descriptors
					// Clean up the entries on the structure, so that this is not used again
					Write(fd_cli_control, file_done, strlen(file_done), &clients[cur_cid]);
					clean_up_client_structure(&clients[cur_cid]);
					// printf("Done\n");
				}
				else if( read_n == -1 )
				{
					// Error in write, client is closed
					Write(fd_cli_control, file_error, strlen(file_error), &clients[cur_cid]);
					clean_up_client_structure(&clients[cur_cid]);
				}
				else
				{
					Write(fd_cli_data, buff, read_n, &clients[cur_cid]);
				}
			}
			/*
				End of the event loop
			*/
		}
	}
}

int main()
{
	/* Find out number of cores */
	num_cores = NO_OF_CORES;

	int total_clients_count = 0;
	/* Change the current working directory to the FILES folder. */
	if( chdir("../FTP_FILES") == -1 )
	{
		perror("CWD");
		exit(0);
	}
	// Register the signal handlers
	signal(SIGINT, sig_term_handler);
	signal(SIGPIPE, sig_pipe_handler);
	// Set resource limits
	set_res_limits();
	// Get and set the default thread priority
	//set_sched_param();
	// This is the listen socket on 21
	int listen_sock = Socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in server_addr;
	server_addr.sin_port = htons(21);
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htons(INADDR_ANY);
	Bind(listen_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
	Listen(listen_sock, BACKLOG);
	

	// Threads
	pthread_t pid;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	struct sched_param sp;
	// Set stack size to 1 MB
 	if( pthread_attr_setstacksize(&attr,1024*1024) < 0 )
	{
		printf("Stack size set error");
		exit(0);
	}
	printf("Stack size set to 512 KB\n");
	// Set the inherit sheduling attr
	/*if( pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED) < 0 )
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
	}*/
	
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
	int master_server = Socket(AF_INET, SOCK_STREAM, 0);
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
			perror("create thread");
        		printf("pthread create error in main\n");
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
	// Monitoring thread
	
	if( pthread_create(&pid, &attr, (void*)monitor, NULL ) != 0 )
        {
        	printf("pthread create error in main");
        	exit(0);
        }
	int client_sock;
	int index;

	/*
		Accept the clients on this thread
	*/
	while( TRUE )
	{
		client_sock = Accept(listen_sock,NULL, NULL);
		if( total_clients_count > CLIENTS_PER_THREAD*TOTAL_NO_THREADS )
		{
			// Refuse the connections
			close(client_sock);
			continue;
		}
		if( client_sock == -1 )
		{
			printf("ERROR in accepting the client\n");
			continue;
		}

		// Got a client.
		//printf("Clients count : %d\n",total_clients_count);
		index = total_clients_count % TOTAL_NO_THREADS;
		// Send to the FD present on the slave array at this index
		Write(slave_fd_array[index], (char*)&client_sock, FD_SIZE, NULL);
		// printf("Written a new client:%d to %d thread\n",client_sock,index);
		increment_clients_count();
		total_clients_count++;
	}
}

