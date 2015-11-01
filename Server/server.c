#include "server.h"

// This is an error checking macro.
#define error_check(res) if (res < 0) { 	\
	printf("Error: %s\n", strerror(errno));	\
	exit(1);								\
}

#define BUFF_SIZE 1024

int heap_a, heap_b, head_c;
server_mode mode;

void update_client(int sock_fd) {
	char buff[2048] = {0, };

	sprintf(buff, "Heap A: %d\nHeap B: %d\nHeap C: %d\n", heap_a, heap_b, head_c);

	error_check(send(sock_fd, buff, sizeof(buff), 0));
}

void client_action(int sock_fd, server_mode *mode) {

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

	error_check((head_c = strtol(argv[3], &end_ptr, 10)));
	
	if (end_ptr == argv[3] || head_c > 1000 || head_c < 0) {
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

	update_client(client_sock_fd);

	while (mode == RUN) {
		//error_check(send(client_sock_fd, "server response test\n", sizeof("server response test\n"), 0));
		update_client(client_sock_fd);

		bzero(buff, BUFF_SIZE); // Clear the buffer before use.
		error_check((byte_num = recv(client_sock_fd, buff, BUFF_SIZE - 1, 0)));

		printf("Size: %d, len: %d\nRecieved: %s\n", byte_num, strlen(buff), buff);

		if (!strcmp(buff, "Q\n")) {
			mode = STOP;
		}
	}

	close(sock_fd);
	close(client_sock_fd);

	return 1;
}