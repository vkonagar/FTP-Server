all: socket_utilities.c common.c main.c protocol.c
	gcc -g -o ftp_server protocol.c common.c socket_utilities.c main.c -pthread
clean:
	rm -f main.o common.o socket_utilities.o protocol.o ftp_server

