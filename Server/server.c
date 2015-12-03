#include "server.h"

// This is an error checking macro.
#define error_check(res) if (res < 0) { 	\
	printf("Error: %s\n", strerror(errno));	\
	exit(1);								\
}

#define BUFF_SIZE 2048
#define MOVE_ERR "Ileagal move\n"
#define MOVE_OK "Move accepted\n"
#define CLIENT_TURN "Your Turn:\n"
#define WIN_SERVER "Server win!\n"
#define CLIENT_WIN "You win!\n"
#define TRUE 1

int heap_a, heap_b, heap_c;
server_mode mode;
char response[BUFF_SIZE];

void prepare_response() {
	char buff[2048] = {0, };

	sprintf(buff, "Heap A: %d\nHeap B: %d\nHeap C: %d\n", heap_a, heap_b, heap_c);

	strcat(response, buff);
}

//this function is to 'decrypt' the client's request
//and perform the client & server move if request is valid.

int client_action(char *request, server_mode *mode) {
	regex_t regex;
	int *curr_heap, remove_num;
	char *end_ptr;
	int regex_res;


	//---------Check if valid command & valid move--------

	if (request[0] == 'Q' && strlen(request) == 1) {
		*mode = STOP;
		return 1;
	}

	//we verify the client's request via regex.
	//the regex is looking for 2 words - a char in the range A-C
	//and a number in the range 0-9999
	error_check(regcomp(&regex, "^[A-C]\\s([0-9]{1,4})$", REG_EXTENDED));

	regex_res = regexec(&regex, request, 0, NULL, 0);
	//printf("%d\n", regex_res);

	//if Invalid command
	if (regex_res == REG_NOMATCH) {
		strcat(response, MOVE_ERR);
		prepare_response();
		strcat(response, CLIENT_TURN);
		regfree(&regex);

		return 0;
	}

	regfree(&regex);

	//Else - Valid command.
	switch (request[0]) {
		case 'A':
			curr_heap = &heap_a;
			break;
		case 'B':
			curr_heap = &heap_b;
			break;
		case 'C':
			curr_heap = &heap_c;
			break;
		default:
			break;
	}

	error_check((remove_num = strtol(&request[2], &end_ptr, 10)));
	
	//----Check if valid move----
	if (end_ptr == &request[2] || remove_num > 1000 || remove_num <= 0 ||
	 	remove_num > *curr_heap) {
		strcat(response, MOVE_ERR);
		prepare_response();
		strcat(response, CLIENT_TURN);

		regfree(&regex);

		return 0;
	}

	//-----------------------------------------------------


	//-------------valid move - Continue with the game-----------
	//client move
	*curr_heap -= remove_num;

	//check client win
	if (heap_a == 0 && heap_b == 0 && heap_c == 0) {
		prepare_response();
		strcat(response, CLIENT_WIN);
		*mode = STOP;

		return 1;
	}
	//-----------------

	//Computer's Move
	curr_heap = &heap_a;

	if (heap_b > heap_a) {
		curr_heap = &heap_b;
	} 
	if (*curr_heap < heap_c) {
		curr_heap = &heap_c;
	}

	*curr_heap -= 1;

	//check computer win
	if (heap_a == 0 && heap_b == 0 && heap_c == 0) {
		prepare_response();
		strcat(response, WIN_SERVER);
		*mode = STOP;

		return 1;
	}
	//-----------------

	strcat(response, MOVE_OK);
	prepare_response();

	strcat(response, CLIENT_TURN);

	return 1;
	//-----------------------------------------------------
}

int main(int argc, char **argv) {
	fd_set master;    // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()
    int fdmax;        // maximum file descriptor number
    int i, j;

	int sock_fd, client_sock_fd;
	socklen_t client_addr_size;
	int byte_num;
	struct sockaddr_in my_addr, client_addr;
	char buff[BUFF_SIZE];

	int server_port = 6444;
	char *end_ptr;

	//-----Validating inputs-----
	assert(argc >= 4 && argc <= 5);

	error_check((heap_a = strtol(argv[1], &end_ptr, 10)));
	
	if (end_ptr == argv[1] || heap_a > 1000 || heap_a < 0) {
		printf("Error: Ileagal heap 1 size given. Please enter a number between 1 and 1000.\n");
		exit(0);
	}


	error_check((heap_b = strtol(argv[2], &end_ptr, 10)));
	
	if (end_ptr == argv[2] || heap_b > 1000 || heap_b < 0) {
		printf("Error: Ileagal heap 2 size given. Please enter a number between 1 and 1000.\n");
		exit(0);
	}

	error_check((heap_c = strtol(argv[3], &end_ptr, 10)));
	
	if (end_ptr == argv[3] || heap_c > 1000 || heap_c < 0) {
		printf("Error: Ileagal heap 3 size given. Please enter a number between 1 and 1000.\n");
		exit(0);
	}


	if (argc == 5) {
		error_check((server_port = strtol(argv[4], &end_ptr, 10)));
		
		if (end_ptr == argv[4] || server_port > 65535 || server_port < 1) {
			printf("Error: Ileagal server port number.\n");
			exit(0);
		}
	}
	//-------------------------

	FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);


	//----Setting up server----
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(server_port);
	my_addr.sin_addr.s_addr = INADDR_ANY;

	sock_fd = socket(PF_INET, SOCK_STREAM, 0);

	error_check(sock_fd);

	error_check(bind(sock_fd, (struct sockaddr *) &my_addr, sizeof(my_addr)));

	// TODO: Check what happens when connections exceeded + change accordingly
	error_check(listen(sock_fd, 1));

	/*** New Select Server ***/

	FD_SET(sock_fd, &master);

	fdmax = sock_fd;

	while(TRUE) {
		read_fds = master; // copy it
		error_check(select(fdmax + 1, &read_fds, NULL, NULL, NULL));

		for(i = 0; i <= fdmax; i++) {
			if (!FD_ISSET(i, &read_fds)) {
				// The FD has no data to read
				continue;
			}

			/**** Handle new connection ****/
			if (i == sock_fd) {
				client_addr_size = sizeof(client_addr);
				error_check((client_sock_fd = accept(sock_fd, (struct sockaddr *) &client_addr, &client_addr_size)));

				// Add new client to set
				// TODO: Send i to client (count clients)
				FD_SET(client_sock_fd, &master);

				if (client_sock_fd > fdmax) {
					fdmax = client_sock_fd;
				}

				continue;
			}

			/**** Handle Requests ****/
			bzero(response, BUFF_SIZE);
			bzero(buff, BUFF_SIZE); // Clear the buffer before use.
			error_check((byte_num = recv(i, buff, BUFF_SIZE - 1, 0)));

			if (byte_num == 0) {
				// TODO: Handle client shutdown
				close(i);
				FD_CLR(i, &master);

				continue;
			}

			for(j = 0; j <= fdmax; j++) {
                // send to everyone!
                if (FD_ISSET(j, &master)) {
                    // except the sock_fd and ourselves
                    if (j != sock_fd && j != i) {
                        if (send(j, buff, byte_num, 0) == -1) {
                            perror("send");
                        }
                    }
                }
            }
		}
	}

	// TODO: close all connections

	//Game Ended - close sockets
	error_check((recv(client_sock_fd, buff, BUFF_SIZE - 1, 0))); //recv shutdown
	close(sock_fd);

	return 1;
}