#include "client.h"

// This is an error checking macro.
#define error_check(res) if (res < 0) { 	\
	printf("Error: %s\n", strerror(errno));	\
	exit(1);								\
}


#define DATASIZE 100

int main(int argc, char **argv) {
	int sockfd, numbytes, rv;
	char buf[DATASIZE];
	struct addrinfo hints, *servinfo, *p;

	char *hostname = "localhost", *port = "6444";

	assert(argc <= 3);
	
	//PRE checks (currently original from Avtachat Meida...)
	/*if (argc != 2) {
		fprintf((stderr,"Usage: client hostname\n"));
		exit(1);
	}
	if ((he = gethostbyname(argv[1])) == NULL) {
		perror("gethostbyname");
		exit(1);
	}
	if ((sockfd = socket(AF_INET,SOCK_STREAM,0)) == -1) {
		perror("socket");
		exit(1);
	}*/



	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if(getaddrinfo(hostname, port, &hints, &servinfo) != 0) {
		perror("getaddrinfo");
		exit(1);
	}

	//loop through possible results
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("connect");
			continue;
		}

		break; //if we got here, we connected succesfully
	}

	if (p == NULL) {
		//looped off the end of the list with no connection
		fprintf(stderr, "failed to connect\n");
		exit(2);
	}

	freeaddrinfo(servinfo);

	if ((numbytes = recv(sockfd,buf, DATASIZE-1,0)) == -1) {
		perror("recv");
		exit(1);
	}

	send(sockfd, "This is Gal's awesome client!!!\n", sizeof("This is Gal's awesome client!!!\n"), 0);

	buf[numbytes] = '\0';
	printf("Received: %s", buf);
	close(sockfd);
	return 0;
}
