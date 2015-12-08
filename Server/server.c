#include "server.h"

int heap_a, heap_b, heap_c;
int clients[CLIENT_NUM];
int client_turn = -1;
server_mode mode;
char response[BUFF_SIZE];

// Time checking variables
time_t turn_start, curr_time;

int main(int argc, char **argv) {
	fd_set master;    // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()
    int fdmax;        // maximum file descriptor number
    int i, j;

	int sock_fd, client_sock_fd, open_cons = 0, client_index;
	socklen_t client_addr_size;
	int byte_num;
	struct sockaddr_in my_addr, client_addr;
	char request[BUFF_SIZE];

	int server_port = 6444;
	char *end_ptr;

	float turn_time;
	struct timeval select_time;

	select_time.tv_sec = 5;
	select_time.tv_usec = 0;

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
	error_check(listen(sock_fd, 10));

	/*** New Select Server ***/

	FD_SET(sock_fd, &master);

	fdmax = sock_fd;

	mode = WAITING_CONS;

	while(mode != STOP) {
		// Check if turn timeout
		if (mode == RUN) {
			curr_time = time(NULL);
			turn_time = (float) curr_time - turn_start;

			printf("in RUN, turn_time: %f\n", turn_time);

			if (turn_time >= 10) {
				printf("turn timeout\n");
			}
		}

		read_fds = master; // copy complete fd set to the sent fd set
		error_check(select(fdmax + 1, &read_fds, NULL, NULL, &select_time));
		// restore timeout struct
		select_time.tv_sec = 5;
		select_time.tv_usec = 0;

		for(i = 0; i <= fdmax; i++) {
			if (!FD_ISSET(i, &read_fds)) {
				// The FD has no data to read
				continue;
			}

			/**** Handle new connection ****/
			if (i == sock_fd) {
				client_addr_size = sizeof(client_addr);
				error_check((client_sock_fd = accept(sock_fd, (struct sockaddr *) &client_addr, &client_addr_size)));

				if (open_cons == CLIENT_NUM) {
					/** Max clients connected, close connection. **/
					error_check(send(client_sock_fd, REJECT_CON, strlen(REJECT_CON), 0));
					shutdown(client_sock_fd, SHUT_WR);
					close(client_sock_fd);
					continue;
				} else if (open_cons < CLIENT_NUM && open_cons > 0) {
					clients[open_cons] = client_sock_fd;
					error_check(send(clients[open_cons], CLIENT2_CONNECT, strlen(CLIENT2_CONNECT), 0));

					FD_SET(client_sock_fd, &master);
					bzero(response, BUFF_SIZE);
					heapStatuses();

					mode = RUN;

					for (client_index = 0; client_index < CLIENT_NUM; client_index++) {
						error_check(send(clients[client_index], response, strlen(response), 0));
					}

					moveToNextTurn();
					
				} else {
					clients[0] = client_sock_fd;
					error_check(send(clients[0], CLIENT1_CONNECT, strlen(CLIENT1_CONNECT), 0));
				}

				open_cons++;

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
			bzero(request, BUFF_SIZE); // Clear the buffer before use.
			error_check((byte_num = recv(i, request, BUFF_SIZE - 1, 0)));

			if (byte_num == 0) {
				// TODO: Handle client shutdown
				close(i);
				FD_CLR(i, &master);

				continue;
			}

			// The loop will find the index of the current client
			// and send it to the action function
			for (j = 0; j < CLIENT_NUM; j++) {
				if (clients[j] == i) {
					client_action(request, &mode, j);
					break;
				}
			}
		}
	}

	// TODO: close all connections

	//Game Ended - close sockets
	error_check((recv(client_sock_fd, request, BUFF_SIZE - 1, 0))); //recv shutdown
	close(sock_fd);

	return 1;
}