all: directory_server file_server1 file_server2 file_server3 client1 client2

directory_server: directory_server.c
	gcc -g -o directory_server directory_server.c -Wall -lnsl -lsocket -lresolv

file_server1: file_server1.c
	gcc -g -o file_server1 file_server1.c -Wall -lnsl -lsocket -lresolv

file_server2: file_server2.c
	gcc -g -o file_server2 file_server2.c -Wall -lnsl -lsocket -lresolv

file_server3: file_server3.c
	gcc -g -o file_server3 file_server3.c -Wall -lnsl -lsocket -lresolv

client1: client1.c
	gcc -g -o client1 client1.c -Wall -lnsl -lsocket -lresolv

client2: client2.c
	gcc -g -o client2 client2.c -Wall -lnsl -lsocket -lresolv

clean:
	rm -f *.o directory_server file_server1 file_server2 file_server3 client1 client2
