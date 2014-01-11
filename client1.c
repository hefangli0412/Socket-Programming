/*
** client.c -- client socket, both UDP and TCP
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

#define UDP2_DS "31000251" // the port client will be connecting to in phase 2
#define UDP2_C "32000251"
#define HOSTNAME "nunki.usc.edu"
#define INFO "Client1 doc1"
#define MAXBUFLEN 100
#define MAXDATASIZE 100 // max number of bytes we can get at once 

char buf_recvfrmdirserv[MAXBUFLEN];

int client_phase2()
{
	struct hostent *he;
	struct in_addr **addr_list;
	int i, err, nbytes_recv;
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	char buf_sendtodirserv[MAXBUFLEN];

	/* get host ip address and print out startup information */
	if ((he = gethostbyname(HOSTNAME)) == NULL) {
		perror("gethostbyname");
		return 2;
	}
	printf("Phase 2: Client 1 has UDP port number %s and IP address ", UDP2_C);
	addr_list = (struct in_addr **)he->h_addr_list;
	for (i = 0; addr_list[i] != NULL; i++)
		printf("%s ", inet_ntoa(*addr_list[i]));
	printf(".\n");

	/*
	 * L49 to L73 are from the given tutorial http://beej.us/guide/bgnet/.
	 */
	/* create socket */
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	if ((err = getaddrinfo("nunki.usc.edu", UDP2_DS, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s", gai_strerror(err));
		return 1;
	}

	// loop through all the results and make a socket
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to bind socket");
		return 2;
	}

	struct sockaddr_in my_addr;
	my_addr.sin_family = AF_UNSPEC;
	my_addr.sin_port = htons(55555);
	my_addr.sin_addr.s_addr = inet_addr(HOSTNAME);
	memset(my_addr.sin_zero, '\0', sizeof my_addr.sin_zero);

	if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof my_addr) < 0)
			perror("bind");
	freeaddrinfo(servinfo);

	/**
	* sent message "Client# doc#" to directory server
	*/
	strcpy(buf_sendtodirserv, INFO);
	if (sendto(sockfd, buf_sendtodirserv, strlen(buf_sendtodirserv), 0, p->ai_addr, p->ai_addrlen) < 0) {
		perror("client: sendto");
		exit(1);
	}
	printf("Phase 2: The File request from Client 1 has been sent to the Directory Server.\n");

	if ((nbytes_recv = recvfrom(sockfd, buf_recvfrmdirserv, MAXBUFLEN-1 , 0,
			NULL, NULL)) == -1) {
		perror("recvfrom");
		exit(1);
	}
	buf_recvfrmdirserv[nbytes_recv] = '\0';
	printf("Phase 2: The File requested by Client 1 is present in <");
	for (i = 0; i < 12; i++)
		printf("%c", buf_recvfrmdirserv[i]);
	printf("> and the File Server's TCP port number is <");
	for (i = 13; i < 21; i++) 
		printf("%c", buf_recvfrmdirserv[i]);
	printf(">.\n");

	close(sockfd);
	printf("Phase 2: End of Phase 2 for Client 1.\n");
	return 0;
}

int client_phase3()
{
	int sockfd;  
	struct addrinfo hints, *servinfo, *p;
	int err, i;
	struct sockaddr_in myaddr;
	socklen_t addrlen;
	char buf_sendtofileserv[MAXBUFLEN];
	char buf_recvfrmfileserv[MAXDATASIZE];
	char *TCP3;

	TCP3 = buf_recvfrmdirserv;
	TCP3 = TCP3 + 21;
	TCP3[0] = '\0';
	TCP3 = buf_recvfrmdirserv;
	TCP3 = TCP3 + 13;

	/*
	 * L128 to L157 are from the given tutorial http://beej.us/guide/bgnet/.
	 */
	/**
	* setup a TCP connection with the file server
	* using the TCP port number obtained from directory server 
	*/
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((err = getaddrinfo("nunki.usc.edu", TCP3, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s", gai_strerror(err));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}

		break;
	}
	if (p == NULL) {
		fprintf(stderr, "client: failed to connect");
		return 2;
	}
	freeaddrinfo(servinfo); // all done with this structure

	/* get host ip address and dynamic TCP port number */
	addrlen = sizeof myaddr;
	if (getsockname(sockfd, (struct sockaddr *)&myaddr, &addrlen) < 0) {
		perror("getsockname");
		return 3;
	}
	printf("Phase 3: Client 1 has dynamic TCP port number %d and IP address %s.\n", (int)ntohs(myaddr.sin_port), inet_ntoa(myaddr.sin_addr));

	/**
	* request for the resource from file server
	*/
	strcpy(buf_sendtofileserv, INFO);
	if (send(sockfd, buf_sendtofileserv, strlen(buf_sendtofileserv), 0) < 0) {
		perror("client: sendto");
		exit(1);
	}
	printf("Phase 3: The File request from Client 1 has been sent to the <");	for (i = 0; i < 12; i++)
		printf("%c", buf_recvfrmdirserv[i]);
	printf(">\n");

	/**
	* receive the resource from file server
	*/
	if (recv(sockfd, buf_recvfrmfileserv, MAXDATASIZE, 0)< 0) {
	    perror("client: recv");
	    exit(1);
	}
	printf("Phase 3: Client 1 received <");
	for (i = 0; i < 4; i++)
		printf("%c", buf_recvfrmfileserv[i]);
	printf("> from <");
	for (i = 0; i < 12; i++)
		printf("%c", buf_recvfrmdirserv[i]);
	printf(">.\n");

	close(sockfd);
	printf("Phase 3: End of Phase 3 for Client 1.\n");
	return 0;
}

int main()
{
	client_phase2();
	client_phase3();

	return 0;
}
