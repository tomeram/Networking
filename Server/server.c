#include "server.h"

// This is an error checking macro.
#define error_check(res) if (res < 0) { 	\
	printf("Error: %s\n", strerror(errno));	\
	exit(1);								\
}

#define BUFF_SIZE 2048
#define MOVE_ERR "Ileagal move:\n"
#define MOVE_OK "Move accepted\n"
#define CLIENT_TURN "Your Turn:\n"
#define WIN_SERVER "Server win!\n"
#define CLIENT_WIN "You win!\n"

int heap_a, heap_b, heap_c;
server_mode mode;
char response[BUFF_SIZE];

void update_client() {
	char buff[2048] = {0, };

	sprintf(buff, "Heap A: %d\nHeap B: %d\nHeap C: %d\n", heap_a, heap_b, heap_c);

	strcat(response, buff);
}

int client_action(char *request, server_mode *mode) {
	regex_t regex;
	int *curr_heap, remove_num;
	char *end_ptr;
	int regex_res;

	if (request[0] == 'Q' && strlen(request) == 1) {
		*mode = STOP;
		return 1;
	}

	error_check(regcomp(&regex, "^[A-C]\\s([0-9]{1,4})$", REG_EXTENDED));

	regex_res = regexec(&regex, request, 0, NULL, 0);
	printf("%d\n", regex_res);

	if (regex_res == REG_NOMATCH) {
		strcat(response, MOVE_ERR);
		update_client();

		regfree(&regex);

		return 0;
	}

	regfree(&regex);

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
	
	if ((end_ptr == &request[2] || remove_num > 1000 || remove_num < 0) && (remove_num < *curr_heap)) {
		strcat(response, MOVE_ERR);

		regfree(&regex);

		return 0;
	}

	*curr_heap -= remove_num;

	if (heap_a == 0 && heap_b == 0 && heap_c == 0) {
		update_client();
		strcat(response, CLIENT_WIN);
		*mode = STOP;

		return 1;
	}

	curr_heap = &heap_a;

	if (heap_b > heap_a) {
		curr_heap = &heap_b;
	} else if (*curr_heap < heap_c) {
		curr_heap = &heap_c;
	}

	*curr_heap -= 1;

	if (heap_a == 0 && heap_b == 0 && heap_c == 0) {
		update_client();
		strcat(response, WIN_SERVER);
		*mode = STOP;

		return 1;
	}

	strcat(response, MOVE_OK);
	update_client();

	strcat(response, CLIENT_TURN);

	return 1;
}

int main(int argc, char **argv) {
	int sock_fd, client_sock_fd;
	int client_addr_size;
	int byte_num;
	struct sockaddr_in my_addr, client_addr;
	char buff[BUFF_SIZE];

	int server_port = 6444;
	char *end_ptr;

	// Validating inputs
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


	// Setting up server
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(server_port);
	my_addr.sin_addr.s_addr = INADDR_ANY; //htonl(0x8443FC64);

	sock_fd = socket(PF_INET, SOCK_STREAM, 0);

	error_check(sock_fd);

	error_check(bind(sock_fd, (struct sockaddr *) &my_addr, sizeof(my_addr)));

	error_check(listen(sock_fd, 1));

	client_addr_size = sizeof(client_addr);

	// The extra beackets are because of the == operator in the error_check macro
	error_check((client_sock_fd = accept(sock_fd, (struct sockaddr *) &client_addr, &client_addr_size)));

	bzero(response, BUFF_SIZE);
	update_client();
	error_check(send(client_sock_fd, response, strlen(response), 0));


	while (mode == RUN) {
		bzero(response, BUFF_SIZE);
		bzero(buff, BUFF_SIZE); // Clear the buffer before use.
		error_check((byte_num = recv(client_sock_fd, buff, BUFF_SIZE - 1, 0)));

		//printf("Size: %d, len: %d\nRecieved: %s\n", byte_num, strlen(buff), buff);

		client_action(buff, &mode);

		error_check(send(client_sock_fd, response, strlen(response), 0));
	}

	close(sock_fd);
	close(client_sock_fd);

	return 1;
}