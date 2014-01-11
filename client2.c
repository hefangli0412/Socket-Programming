/*
** client2.c -- client socket, both UDP and TCP
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

#define UDP2_DS "31251" // the port client will be connecting to in phase 2
#define UDP2_C 33251
#define UDP2_CL "33251"
#define HOSTNAME "nunki.usc.edu"
#define INFO "Client2 doc2"
#define MAXBUFLEN 100
#define MAXDATASIZE 100 // max number of bytes we can get at once 

char buf_recvfrmdirserv[MAXBUFLEN];

int client_phase2()
{
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_in my_addr, print_addr;
	socklen_t addrlen;
	struct hostent *he;
	struct in_addr **addr_list;
	int i, err, nbytes_recv;
	char buf_sendtodirserv[MAXBUFLEN];

	/*
	 * L40 to L62 are from the given tutorial http://beej.us/guide/bgnet/.
	 */
	/* create socket */
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	if ((err = getaddrinfo(HOSTNAME, UDP2_DS, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s", gai_strerror(err));
		return 1;
	}

	/* loop through all the results and make a socket */
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}
		break;
	}
	if (p == NULL) {
		fprintf(stderr, "client: failed to bind socket");
		return 2;
	}

	/* configure port number using bind() */
	my_addr.sin_family = AF_UNSPEC;
	my_addr.sin_port = htons(UDP2_C);
	my_addr.sin_addr.s_addr = inet_addr(HOSTNAME);
	memset(my_addr.sin_zero, '\0', sizeof my_addr.sin_zero);

	if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof my_addr) < 0)
		perror("bind");

	/* get host ip address and port number and print out startup information*/
	if ((he = gethostbyname(HOSTNAME)) == NULL) {
		perror("gethostbyname");
		return 3;
	}

	addrlen = sizeof print_addr;
	if (getsockname(sockfd, (struct sockaddr *)&print_addr, &addrlen) < 0) {
		perror("getsockname");
		return 4;
	}

	printf("Phase 2: Client 2 has UDP port number %d and IP address ", (int)ntohs(print_addr.sin_port));
	addr_list = (struct in_addr **)he->h_addr_list;
	for (i = 0; addr_list[i] != NULL; i++)
		printf("%s ", inet_ntoa(*addr_list[i]));
	printf(".\n");

	freeaddrinfo(servinfo); // all done with this structure

	/* sent message "Client# doc#" to directory server */
	strcpy(buf_sendtodirserv, INFO);
	if (sendto(sockfd, buf_sendtodirserv, strlen(buf_sendtodirserv), 0, p->ai_addr, p->ai_addrlen) < 0) {
		perror("client: sendto");
		exit(1);
	}
	printf("Phase 2: The File request from Client 2 has been sent to the Directory Server.\n");

	close(sockfd);

	/*
	 * L106 to L132 are from the given tutorial http://beej.us/guide/bgnet/.
	 */
	/* create socket */
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	if ((err = getaddrinfo(HOSTNAME, UDP2_CL, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s", gai_strerror(err));
		return 1;
	}

	/* loop through all the results and make a socket */
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: bind");
			continue;
		}
		break;
	}
	if (p == NULL) {
		fprintf(stderr, "client: failed to bind socket");
		return 2;
	}
	freeaddrinfo(servinfo);

	/* receive message of file server from directory server */
	if ((nbytes_recv = recvfrom(sockfd, buf_recvfrmdirserv, MAXBUFLEN - 1 , 0, p->ai_addr, &p->ai_addrlen)) == -1) {
		perror("recvfrom");
		exit(1);
	}
	buf_recvfrmdirserv[nbytes_recv] = '\0';
	printf("Phase 2: The File requested by Client 2 is present in <");
	for (i = 0; i < 12; i++)
		printf("%c", buf_recvfrmdirserv[i]);
	printf("> and the File Server's TCP port number is <");
	for (i = 13; i < 18; i++) 
		printf("%c", buf_recvfrmdirserv[i]);
	printf(">.\n");

	close(sockfd);
	printf("Phase 2: End of Phase 2 for Client 2.\n");

	return 0;
}

int client_phase3()
{
	int sockfd;  
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_in myaddr;
	socklen_t addrlen;
	int err, i;
	char buf_sendtofileserv[MAXBUFLEN];
	char buf_recvfrmfileserv[MAXDATASIZE];
	char *TCP3;

	TCP3 = buf_recvfrmdirserv;
	TCP3 = TCP3 + 18;
	TCP3[0] = '\0';
	TCP3 = buf_recvfrmdirserv;
	TCP3 = TCP3 + 13;

	/*
	 * L175 to L203 are from the given tutorial http://beej.us/guide/bgnet/.
	 */
	/* setup a TCP connection with the file server using the TCP port number obtained from directory server */
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((err = getaddrinfo(HOSTNAME, TCP3, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s", gai_strerror(err));
		return 1;
	}

	/* loop through all the results and connect to the first we can */
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
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
	freeaddrinfo(servinfo);

	/* get host ip address and dynamic TCP port number and print out starup information */
	addrlen = sizeof myaddr;
	if (getsockname(sockfd, (struct sockaddr *)&myaddr, &addrlen) < 0) {
		perror("getsockname");
		return 3;
	}
	printf("Phase 3: Client 2 has dynamic TCP port number %d and IP address %s.\n", (int)ntohs(myaddr.sin_port), inet_ntoa(myaddr.sin_addr));

	/* request for the resource from file server */
	strcpy(buf_sendtofileserv, INFO);
	if (send(sockfd, buf_sendtofileserv, strlen(buf_sendtofileserv), 0) < 0) {
		perror("client: sendto");
		exit(1);
	}
	printf("Phase 3: The File request from Client 2 has been sent to the <");
	for (i = 0; i < 12; i++)
		printf("%c", buf_recvfrmdirserv[i]);
	printf(">\n");

	/* receive the resource from file server */
	if (recv(sockfd, buf_recvfrmfileserv, MAXDATASIZE - 1, 0) < 0) {
	    perror("client: recv");
	    exit(1);
	}
	printf("Phase 3: Client 2 received <");
	for (i = 0; i < 4; i++)
		printf("%c", buf_recvfrmfileserv[i]);
	printf("> from <");
	for (i = 0; i < 12; i++)
		printf("%c", buf_recvfrmdirserv[i]);
	printf(">.\n");

	close(sockfd);
	printf("Phase 3: End of Phase 3 for Client 2.\n");

	return 0;
}

int main()
{
	client_phase2();
	client_phase3();

	return 0;
}
