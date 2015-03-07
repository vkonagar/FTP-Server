import ftplib
import sys
from threading import Thread
import time
import traceback

def threaded_function(file_name,host,passive):
	user = "user"
	passw = "freebsd";
	
	ftp = None;
	while True:
		try:
			ftp = ftplib.FTP( host, user, passw, timeout=6000 )
			print "logged for "+str(file_name)
			break
		except Exception:
			print traceback.format_exc()
			#print "\n\n Cant open conection "+file_name+" at client 1 \n\n"
			print "Continuing\n"
			continue

	#Active
	ftp.set_pasv(False);
	
	filee = open(file_name, 'w')
	
	while True:
		try:
			ftp.retrbinary('RETR '+file_name, filee.write)
			break
		except Exception:
			print traceback.format_exc()
			print "Cant initiatte data connection from client 1 for  "+file_name
			time.sleep(2)
			continue
	#ftp.quit()
	
	exit();


threads = []

if __name__ == "__main__":
	
	############ ARGS ##############
	# 1) file count to be fetched (active)
	# 2) file count to be fetched (passive)
	# 3) host name
	# 4) file name
	################################

	if( len(sys.argv) != 6 ):
		print "Wrong number of args\n"
		exit();

	# Arguments are number of files to be fetched

	file_count = int(sys.argv[5]);
	# ACTIVE
	for i in range(1,int(sys.argv[1])+1):
		time.sleep(0.01);
                thread = Thread(target = threaded_function, args = (sys.argv[4]+str(file_count),sys.argv[3],False))
		thread.start()
		threads.append(thread)
		file_count+=1
	# PASSIVE
	#for i in range(1,int(sys.argv[2])+1):
	#	
	#	thread = Thread(target = threaded_function, args = (sys.argv[4]+str(file_count),sys.argv[3],True))
	#	thread.start()
	#	threads.append(thread)
	#	file_count+=1
    	
	for i in threads:
		i.join()
    	print "All threads finished...exiting"
    	
