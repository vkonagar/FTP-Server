# IO-Multiplexed-threaded-FTP-server
* This is a Scalable FTP server implementation, which uses IO multiplexing to handle large number of clients at a time for basic FTP file fetching.
* It is better than conventional FTP servers (ex: vsftpd) in terms of scalability of connections.
* It is used for testing the scalablility of the FTP server or FTP ALG on any middlebox device like NAT. 
* vsftpd spawns a process for every client and pose a significant overhead for scheduling and memory. This architecture will eliminate the overhead by pre-spawning a predefined no of worker threads and main thread.
* Main thread load balances incoming client requests to workers.

### Algorithm
* Rather than using one thread per client, I have used one thread for many clients ( say 200 ). This will reduce the scheduling overhead on the OS.
* Main thread will accept the client FTP requests on the FTP socket 21. After it accepts an fd, it redirects that fd to a worker thread through internal Inter-thread sockets.
* Initially, worker threads register themselves with the main thread by connecting to an internal TCP server run by main thread. After the connections are established, main thread will get file descriptors for each of the worker threads connected to its internal TCP server. Main thread will send the newly accepted client file descriptors by writing on the worker thread fd.
* Worker thread grabs the new client fd by reading the internal socket.
* Now, worker thread will initiate the connection back to clients for data connection and also open files for reading.
* At any time, worker thread can have more than 1 clients connected to it. It handles them by using the epoll sys call.
* Each worker thread listens on many file descriptors at a time by using epoll system call. Epoll is better than select or poll and is helpful for scaling, as there is significant overhead in copying fds with select or poll system calls.
* There are three fds for every client connected to the FTP server(worker thread). client control connection fd, data connection fd, file fd for the requested file.
* Epoll system call on a worker thread will wait for read events on the control connection fds of all the clients that it handles. If any event is generated by any client, then that client is served. 
* If a file is requested in an event, then the file is opened and a connection is initiated back to the client to serve the file.
* If the data connection fd of a client is ready for writing, then in every loop a block of data is read from the file and written to the data connection fd.
* This is continued until all of the data connections are served with their respected files. After that, all of the control, data and file fds of a client are closed.
* Level triggered IO is set for the epoll system call to select on all of the client control conenction fds and the master fd.
* After each call to epoll_wait, A block is read from the respective file and written to the data conenction fd. 
* No polling is required for the data conenctions, as they are always ready to write.

#### Algorithm for the event loop
```
loop start
epoll_wait()
  for each of ready descriptors:
    serve_the_fds()
  serve_file_IO()
loop end
  
 ```

### How to run

1. Download the directory.
2. add a folder FTP_FILES in the above directory to the current directory where the code is present.
3. Put all the files to be served in this FTP_FILES directory
4. Open common.h and set the params for CLIENTS_PER_THREAD and NO_OF_THREADS
5. Compile using 'make'
6. run ftp_server by ./ftp_server
7. Now ftp server is ready to serve the files in the FTP_FILES directory. 

### Notes
* This ftp server can only serve files in the FTP_FILES directory. This is for a reason as I have not created a production ftp server, but merely implemented it for testing scalability.
* It cant handle change directory(chdir) FTP command.
* It only uses active mode for now. Passive mode would be added later.
* Username and password are not required. Server simply discards them after getting it.
* Only binary transfer mode is supported.
