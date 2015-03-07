# IO-Multiplexed-threaded-FTP-server
Implementation of a FTP server, which is multiplexed and can handle many clients at a time for basic file fetching.

### Architecture
* Rather than using one thread per client, i used one thread for many clients ( say 200 ). This will reduce the scheduling overhead on the OS.
* Main thread will accept the clients on the FTP socket 21. After it accepts an fd, it redirects that fd to a thread accordingly.
* Initially, threads will register with the main thread by connecting to an internal TCP server run by main thread. Threads connect to that internal server and main thread gets an FD for that thread, which is used for later communication. Main thread will send the newly accepted client fds through this thread connection fds.
* Each thread will listen on many file descriptors at a time by using epoll system call. This system call is better than select and poll and will scale linearly as the fds increase.
* There are three fds for every client. client control connection fd, data connection fd, file fd.
* Epoll system call will wait for read events on the control connection of all the clients of that thread. If any event is generated, then it is served. One of the event is a request to a file, then a data connection is established and added to the listen fds for writing. 
* If the data connection fd of a client is ready for writing, then everytime a block of data is read from the file and transferred to the data connection fd.
* This is continued until all of the data connections are served for complete files. then all of the control, data and file fd of a client are closed.
* Level triggered IO is set for the epoll system call to select on all of the client control conenction fds and the master fd.
* After each call to epoll_wait, IO is done for the clients from the file_fd to the data_connection_fd. NO polling is required for the data conenctions, as they are always ready to write. Data is read from the file_fd and written to the data_con_fd.

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
3. Put all the files in this FTP_FILES directory
4. Open common.h and set the params for CLIENTS_PER_THREAD and NO_OF_THREADS
5. Compile using 'make'
6. run ftp_server by ./ftp_server
7. Now ftp server is ready to serve the files in the FTP_FILES directory. 

### Notes
* This ftp server can only server files in the FTP_FILES directory. This is for a reason, as i have not created a production ftp server, but merely to test the scalability.
* It doesn't have change directoy command.
* It can only use active mode for now. Passive mode to be added later.
* Username and password are not required and you can use anything there. Server simply discards them after getting it.
* Only binary mode is supported.
