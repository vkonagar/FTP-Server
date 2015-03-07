all:
	gcc -g -o ftp_server common.h socket_utilities.h protocol.h main.c -pthread

