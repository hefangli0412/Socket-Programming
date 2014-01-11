/*
** directory_server.c -- directory server socket, only UDP
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

#define UDP1 "21000251"	// the port file server will be connecting to in phase 1
#define UDP2 "31000251" // the port client will be connecting to in phase 2
#define HOSTNAME "nunki.usc.edu"
#define MAXBUFLEN 100

int StoreFileservInfo(char *buf)
{
	FILE *fp = 0;
	int pos;
	char fileline[MAXBUFLEN];
	int flag = 0;	
	
	fp = fopen("directory.txt", "r+");
	if (fp == 0)
		fp = fopen("directory.txt", "a+");
	pos = ftell(fp);

	while (fgets(fileline, sizeof fileline, fp) != 0) { 
		if (strncmp(fileline, buf, 12) == 0 ) {
			fseek(fp, pos, SEEK_SET);
			fputs(buf, fp);
			fputc('\n', fp);
			pos = ftell(fp);
			flag++;
			break;
		} else {
			pos = ftell(fp);
			continue;
		}
	}

	if (flag == 0) {
		fputs(buf, fp);
		fputc('\n', fp);
	}

	fclose(fp);
	return 0;
}

int FindPossibleFileserv(char *docinfo, int *flag) {
	FILE *fp;
	char fileline[MAXBUFLEN];
	char *p, *q;
	int i, j, n;

	if ((fp = fopen("resource.txt", "r")) == 0) {
		printf("Could not open file directory.txt\n");
		return -1;
	}

	for (i = 0; q != 0; i++) {
		q = fgets(fileline, sizeof fileline, fp);
		p = strchr(fileline, ' ');
		p++;
		n = p[0] - 48;

		for (j = 0; j < n; j++) {
			p = strchr(p, ' ');
			p++;
			if (strncmp(p, docinfo, 4) == 0) {
				flag[i] = 1;
				break;	
			}
		}
	}

	fclose(fp);
	return 0;
}

int FindNearestFileserv(char *clientinfo, int *flag) {
	FILE *fp;
	char fileline[MAXBUFLEN];
	char *str = "Client1";
	char *p;
	int i, ret;
	int tmp = 0;

	if (flag[0]+flag[1]+flag[2] == 0) {
		printf("cannot find the corresponding file server\n");
		return -1;
	}

	if ((fp = fopen("topology.txt", "r")) == 0) {
		printf("Could not open file topology.txt\n");
		return -1;
	}

	if (strncmp(str, clientinfo, 7) == 0) {
		fgets(fileline, sizeof fileline, fp);
	} else {
		fgets(fileline, sizeof fileline, fp);
		fgets(fileline, sizeof fileline, fp);
	}

	p = fileline;		
	for (i = 0; i < 3; i++) {
		if (flag[i] == 1) {
			if (tmp == 0) {
				tmp = atoi(p);
				ret = 0;
			} else {
				if (atoi(p) < tmp) {
					tmp = atoi(p);
					ret = i;
				}
			}
		}
        	p = strchr(p, ' ');
		p++;
	}

	return (ret + 1);
	fclose(fp);
}

int Findfileservinfo(char *serverinfo, int ret) {
	FILE *fp;
	char fileline[MAXBUFLEN];
	char *p;

	if ((fp = fopen("directory.txt", "r")) == 0) {
		printf("Could not open file directory.txt");
		return -1;
	}

	while (fgets(fileline, sizeof fileline, fp) != 0) { 
		p = fileline;
		p = p + 11;
		if (ret == p[0] - 48) {
			strcpy(serverinfo, fileline);
			break;
		}
	}

	fclose(fp);
	return 0;
}

int dirc_phase1()
{
	int err, nbytes;
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr;
	socklen_t addr_len;
	char buf_recvfrmfileserv[MAXBUFLEN];

	/*
	 * L169 to L197 are from the given tutorial http://beej.us/guide/bgnet/.
	 */
	/* create socket */
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((err = getaddrinfo(NULL, UDP1, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s", gai_strerror(err));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("directory server: socket");
			continue;
		}
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("directory server: bind");
			continue;
		}
		break;
	}
	if (p == NULL) {
		fprintf(stderr, "directory server: failed to bind socket");
		return 2;
	}
	freeaddrinfo(servinfo);
	
	/**
	* get static TCP port number from file servers
	* and store it in directory.txt 
	*/
	addr_len = sizeof their_addr;
	if ((nbytes = recvfrom(sockfd, buf_recvfrmfileserv, MAXBUFLEN-1 , 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
		perror("recvfrom");
		exit(1);
	}
	buf_recvfrmfileserv[nbytes] = '\0';
	printf("Phase 1: The Directory Server has received request from File Server %c.\n", buf_recvfrmfileserv[11]);

	StoreFileservInfo(buf_recvfrmfileserv);

	close(sockfd);
	return 0;
}

int dirc_phase2()
{
	int err, ret;
	int flag[3] = {0, 0, 0};
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr;
	socklen_t addr_len;
	char buf_recvfrmclient[MAXBUFLEN];
	char clientinfo[MAXBUFLEN];
	char docinfo[MAXBUFLEN];
	char serverinfo[MAXBUFLEN];
	char buf_sendtoclient[MAXBUFLEN];

	/*
	 * L234 to L263 are from the given tutorial http://beej.us/guide/bgnet/.
	 */
	/* create socket */
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((err = getaddrinfo(NULL, UDP2, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s", gai_strerror(err));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("directory server: socket");
			continue;
		}
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("directory server: bind");
			continue;
		}
		break;
	}
	if (p == NULL) {
		fprintf(stderr,	"directory server: failed to bind socket");
		return 2;
	}
	freeaddrinfo(servinfo);

	/**
	* get the resource the client is looking for
	* check for its nearest file server containing the resource
	* send TCP port number of the desired file server to the client 
	*/
	addr_len = sizeof their_addr;
	if (recvfrom(sockfd, buf_recvfrmclient, MAXBUFLEN , 0, (struct sockaddr *)&their_addr, &addr_len) < 0) {
		perror("recvfrom");
		exit(1);
	}
	printf("Phase 2: The Directory Server has received request from Client %d.\n", buf_recvfrmclient[6] - 48);

	strncpy(clientinfo, buf_recvfrmclient, 7);
	clientinfo[7] = '\0';
	strncpy(docinfo, &buf_recvfrmclient[8], 4);
	docinfo[4] = '\0';

	FindPossibleFileserv(docinfo, flag);
	ret = FindNearestFileserv(clientinfo, flag);
	Findfileservinfo(serverinfo, ret);	

	strcpy(buf_sendtoclient, serverinfo);
	if (sendto(sockfd, buf_sendtoclient, strlen(buf_sendtoclient), 0, (struct sockaddr *)&their_addr, addr_len) < 0) {
		perror("directory server: sendto");
		exit(1);
	}
	printf("Phase 2: File server details has been sent to Client%d.\n", buf_recvfrmclient[6] - 48);

	close(sockfd);
	return 0;
}

int main(void)
{		
	int i;
	struct hostent *he;
	struct in_addr **addr_list;

	/* get host ip address and print out startup information */
	if ((he = gethostbyname(HOSTNAME)) == NULL) {
		perror("gethostbyname");
		return 2;
	}
	printf("Phase 1: The Directory Server has UDP port number %s and IP address ", UDP1);
	addr_list = (struct in_addr**)he->h_addr_list;
	for (i = 0; addr_list[i] != NULL; i++) 
		printf("%s ", inet_ntoa(*addr_list[i]));
	printf(".\n");

	/* implement phase 1 for 3 times */
	for (i = 0; i < 3; ++i) {	
		dirc_phase1();
	}
	printf("Phase 1: The directory.txt file has been created.\nEnd of Phase 1 for the Directory Server.\n");
	
	/* get host ip address and print out startup information */
	if ((he = gethostbyname(HOSTNAME)) == NULL) {
		perror("gethostbyname");
		return 2;
	}
	printf("Phase 2: The Directory Server has UDP port number %s and IP address ", UDP2);
	addr_list = (struct in_addr**)he->h_addr_list;
	for (i = 0; addr_list[i] != NULL; i++) 
		printf("%s ", inet_ntoa(*addr_list[i]));
	printf(".\n");

	/* implement phase 2 twice */
	for (i = 0; i < 2; ++i) {
		dirc_phase2();
	}
	printf("Phase 2: End of Phase 2 for the Directory Server.\n");

	return 0;
}
