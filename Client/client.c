#include "client.h"

// This is an error checking macro.
#define error_check(res) if (res < 0) { 	\
	printf("Error: %s\n", strerror(errno));	\
	exit(1);								\
}


#define BUFF_SIZE 1024

int main(int argc, char **argv) {
	int sockfd, numbytes;
	char response[BUFF_SIZE], request[BUFF_SIZE];
	char *default_hostname = "localhost", *default_port = "6444", 
			*hostname, *port;
	char input[101], *end_check = NULL; //limit user to 100 chars to send

	mode game_mode;

	struct addrinfo hints, *servinfo, *p;

	assert(argc <= 3 && " Error: Too many arguments given...");
	
	//-------------------PRE checks--------------------
	switch(argc) {
		case 1: 
			hostname = default_hostname;
			port 	 = default_port;
			break;
		case 2:
			hostname = argv[1];
			port 	 = default_port;
			break;
		case 3:
			hostname = argv[1];
			port 	 = argv[2];
			break;
	}
	//-------------------------------------------------


	//-----------Get possible server address-----------
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if(getaddrinfo(hostname, port, &hints, &servinfo) != 0) {
		perror(" Can't resolve address info. Exitting");
		exit(1);
	}
	//-------------------------------------------------

	//--loop through possible results. try to connect--
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			/*perror(" Can't initiate socket.");
			if (p->ai_next != NULL) {
				printf("tring another connection");
			}*/
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			//perror(" Failed to connect.");
			continue;
		}

		break; //if we got here, we connected succesfully
	}

	freeaddrinfo(servinfo);

	if (p == NULL) {
		//looped off the end of the list with no connection
		fprintf(stderr, " Failed to connect. Exitting\n");
		exit(2);
	}
	//-------------------------------------------------
	

	//-----------------Connected - Game----------------
	game_mode = RUN;
	while (game_mode == RUN) {

		//-----Recv & Print-------------
		bzero(response, BUFF_SIZE);
		if ((numbytes = recv(sockfd,response, BUFF_SIZE-1,0)) == -1) {
			perror(" Error recv");
			exit(1);
		}

		response[numbytes] = '\0';
		printf("%s\n", response);
		//------------------------------


		//------Check if game ended-----
		end_check = strstr(response, "win!");
		if (end_check != NULL) {
			game_mode = STOP;
			break;
		}
		//-----------------------------


		//-----Scanf & Send------------
		//the request is a string of the command - exactly as the user typed them.
		//validation of the command is done server-side, with the exception of "Q",
		//that if found by the client - sends a shutdown signal to the server.
		//the response is simply a string of the game status
		//the game logic & status is kept on the server only.
		bzero(request ,BUFF_SIZE);

		bzero(input, 101);
		if (scanf("%100s", input) == 100) {
			input[101] = '\0';
		}

		//--Check if user ended game--
		if (input[0] == 'Q' && input[1] == '\0') {
			game_mode = STOP;
			error_check(send(sockfd, input, sizeof(input), 0));
			break;
		}
		//-----------------------------
		
		strcpy(request, input);
		strcat(request, " ");

		bzero(input, 101);
		if (scanf("%100s", input) == 100) {
			input[101] = '\0';
		}

		strcat(request, input);

		error_check(send(sockfd, request, strlen(request), 0));		
		//-----------------------------
		
	}
	//-------------------------------------------------

	shutdown(sockfd, SHUT_WR);

	close(sockfd);
	return 1;
}
