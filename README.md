# IO-Multiplexed-threaded-FTP-server
Implementation of a FTP server, which is multiplexed and can handle many clients at a time for basic file fetching.

### Architecture
* Level triggered IO is set for the epoll system call to select on all of the client control conenction fds and the master fd.
* After each call to epoll_wait, IO is done for the clients from the file_fd to the data_connection_fd. NO polling is required for the data conenctions, as they are always ready to write. Data is read from the file_fd and written to the data_con_fd.
```
loop start
epoll_wait()
  for each of ready descriptors:
    serve_the_fds()
  serve_file_IO()
loop end
  
 ```
