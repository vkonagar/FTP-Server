import socket
import time

sockets = []
count = 1025
while(1):
    for res in socket.getaddrinfo("15.15.15.1", count, socket.AF_INET, socket.SOCK_STREAM, 0, socket.AI_PASSIVE):
                af, socktype, proto, canonname, sa = res
                sock = socket.socket(af, socktype, proto)
                sock.bind(sa)
                sock.listen(1)
                sockets.append(sock)
                port = sock.getsockname()[1]
                host = sock.getsockname()[0]
                print "Host : "+str(host)+"\n"
                print "Port : "+str(port)+"\n\n"
    count+=1
    for res in socket.getaddrinfo("17.17.17.1", count, socket.AF_INET, socket.SOCK_STREAM, 0, socket.AI_PASSIVE):
                af, socktype, proto, canonname, sa = res
                sock = socket.socket(af, socktype, proto)
                sock.bind(sa)
                sock.listen(1)
                sockets.append(sock)
                port = sock.getsockname()[1]
                host = sock.getsockname()[0]
                print "Host : "+str(host)+"\n"
                print "Port : "+str(port)+"\n\n"
    count+=1
    time.sleep(1);
    print "\n"+str(count)+"\n"
