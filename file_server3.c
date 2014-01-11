/*
** file_server.c -- file server socket, both UDP and TCP
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/wait.h>
#include <signal.h>

#define UDP1_DS "21000251" //the port file server will be connecting to in phase 1
#define UDP1_FS "24000251"
#define TCP3 "43000251" // the port client will be connecting to in phase 3
#define HOSTNAME "nunki.usc.edu"
#define INFO "File_Server3 43000251"
#define MAXBUFLEN 100
#define BACKLOG 10	 // how many pending connections queue will hold

void sigchld_handler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

int file_phase1()
{
	struct hostent *he;
	struct in_addr **addr_list;
	int i, err, nbytes;
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	char buf_sendtodirserv[MAXBUFLEN];

	/* get host ip address and print out startup information */
	if ((he = gethostbyname(HOSTNAME)) == NULL) {
		perror("gethostbyname");
		return 2;
	}
	printf("Phase 1: File Server 3 has UDP port number %s and IP address ", UDP1_FS);
	addr_list = (struct in_addr **)he->h_addr_list;
	for (i = 0; addr_list[i] != NULL; i++)
		printf("%s ", inet_ntoa(*addr_list[i]));
	printf(".\n");

	/*
	 * L54 to L77 are from the given tutorial http://beej.us/guide/bgnet/.
	 */
	/* create socket */
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	if ((err = getaddrinfo("nunki.usc.edu", UDP1_DS, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s", gai_strerror(err));
		return 1;
	}

	// loop through all the results and make a socket
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("file server: socket");
			continue;
		}
		break;
	}
	if (p == NULL) {
		fprintf(stderr, "file server: failed to bind socket");
		return 2;
	}
	freeaddrinfo(servinfo);

	/**
	* register with the directory server by
	* sending "File_Server# TCP_port_number"
	*/
	strcpy(buf_sendtodirserv, INFO);
	if ((nbytes = sendto(sockfd, buf_sendtodirserv, strlen(buf_sendtodirserv), 0, p->ai_addr, p->ai_addrlen)) == -1) {
		perror("file server: sendto\n");
		exit(1);
	}
	printf("Phase 1: The Registration request from File Server 3 has been sent to the Directory Server.\n");

	close(sockfd);
	printf("Phase 1: End of Phase 1 for File Server 3.\n");

	return 0;
}

int file_phase3()
{
	struct hostent *he;
	struct in_addr **addr_list;
	int i, err, nbytes_recv;
	int sockfd, newfd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes = 1;
	char buf_recvfrmclient[MAXBUFLEN];
	char buf_sendtoclient[MAXBUFLEN];

	/* get host ip address and print out startup information */
	if ((he = gethostbyname(HOSTNAME)) == NULL) {
		perror("gethostbyname");
		return 2;
	}
	printf("Phase 3: File Server 3 has TCP port %s and IP address ", TCP3);
	addr_list = (struct in_addr **)he->h_addr_list;
	for (i = 0; addr_list[i] != NULL; i++)
		printf("%s ", inet_ntoa(*addr_list[i]));
	printf(".\n");

	/*
	 * L125 to L189 are from the given tutorial http://beej.us/guide/bgnet/.
	 */
	/* create TCP socket */
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((err = getaddrinfo(NULL, TCP3, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s", gai_strerror(err));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("file server: socket");
			continue;
		}
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("file server: setsockopt");
			exit(1);
		}
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("file server: bind");
			continue;
		}
		break;
	}
	if (p == NULL)  {
		fprintf(stderr, "file server: failed to bind");
		return 2;
	}
	freeaddrinfo(servinfo); // all done with this structure

	/**
	* open TCP connection 
	* and start listening on static TCP port
	*/
	if (listen(sockfd, BACKLOG) == -1) {
		perror("file server: listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("file server: sigaction");
		exit(1);
	}

	/**
	* accept TCP connection from the client
	*/
	while(1) {  // main accept() loop
		sin_size = sizeof their_addr;
		newfd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (newfd == -1) {
			perror("file server: accept");
			continue;
		}

		if (!fork()) { // this is the child process
			close(sockfd); // child doesn't need the listener
			/**
			* check the resource the client is looking for
			*/
			if ((nbytes_recv = recv(newfd, buf_recvfrmclient, MAXBUFLEN, 0)) < 0) 
	    			perror("file server: recv");
			buf_recvfrmclient[nbytes_recv] = '\0';
			printf("Phase 3: File Server 3 received the request from the <");
			for (i = 0; i < 7; i++)
				printf("%c", buf_recvfrmclient[i]);
			printf("> for the file <");
			for (i = 8; i < 12; i++)
				printf("%c", buf_recvfrmclient[i]);
			printf(">.\n");

			/**
			* send the resource to the client
			*/
			strcpy(buf_sendtoclient, &buf_recvfrmclient[8]);
			if (send(newfd, buf_sendtoclient, strlen(buf_sendtoclient), 0) < 0)
				perror("file server: send\n");
			printf("Phase 3: File server 3 has sent <");
			for (i = 0; i < 4; i++)
				printf("%c", buf_sendtoclient[i]);
			printf("> to <");
			for (i = 0; i < 7; i++)
				printf("%c", buf_recvfrmclient[i]);
			printf(">.\n");

			close(newfd);
			exit(0);
		}

		close(newfd);  // parent doesn't need this
	}

	return 0;
}

int main()
{
	file_phase1();
	file_phase3();
	
	return 0;
}
