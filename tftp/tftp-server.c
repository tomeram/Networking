#include "tftp-server.h"

// This is an error checking macro.
#define error_check(res) if (res < 0) { 	\
	printf("Error: %s\n", strerror(errno));	\
	exit(1);								\
}


#define BUFF_SIZE 1024
#define PORT_NUM 69

int sockfd;
char request[BUFF_SIZE];
int req_len;

int main(int argc, char **argv) {
	struct sockaddr_in serveraddr, clientaddr;

	int clientlen = sizeof(struct sockaddr_in);
	int optval; /*seems to be handy trick to avoid waiting the error "Address already in use"*/

	if (argc != 1) {
		printf("Too many arguments given\n");
		return 0;
	}

	/*--Create socket--*/
	sockfd = socket(AF_INET, SOCK_DGRAM,IPPROTO_UDP);
	error_check(sockfd);

	/*
	taken from google:
	setsockopt: Handy debugging trick that lets us return the server immediately after we kill it;
	otherwise we have to wait about 20 secs.
	eliminates "Error on binding: Address already in use" error.
	*/
	optval = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));

	/* build server adress */
	bzero((char*) &serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(PORT_NUM);
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

	/*--Bind Socket--*/
	error_check(bind(sockfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)));

	while (1) {
		bzero(request, BUFF_SIZE);
		req_len = recvfrom(sockfd, request, BUFF_SIZE, 0, (struct sockaddr *) &clientaddr, &clientlen);
		error_check(req_len); //TODO - don't need to quit here... need to allow another client
		
		printf("Received from %s:%d: \n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
		printf("Received %d bytes: %s\n", req_len,request);
	}
}

