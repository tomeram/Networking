#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

// This is an error checking macro.
#define error_check(res) if (res < 0) { 	\
	printf("Error: %s\n", strerror(errno));	\
	exit(1);								\
}

#define BUFF_SIZE 1024

int main(int argc, char **argv) {
	int sock_fd, client_sock_fd;
	int client_addr_size;
	int byte_num;
	struct sockaddr_in my_addr, client_addr;
	char buff[BUFF_SIZE];

	int heap_a, heap_b, head_c;
	char *end_ptr;

	// Validating inputs
	assert(argc == 4);

	error_check((heap_a = strtol(argv[1], &end_ptr, 10)));
	
	if (end_ptr == argv[1]) {
		printf("Error: Ileagal heap 1 size given.\n");
		exit(0);
	}

	error_check((heap_b = strtol(argv[2], &end_ptr, 10)));
	
	if (end_ptr == argv[2]) {
		printf("Error: Ileagal heap 2 size given.\n");
		exit(0);
	}

	error_check((head_c = strtol(argv[3], &end_ptr, 10)));
	
	if (end_ptr == argv[3]) {
		printf("Error: Ileagal heap 3 size given.\n");
		exit(0);
	}


	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(80);
	my_addr.sin_addr.s_addr = INADDR_ANY; //htonl(0x8443FC64);

	sock_fd = socket(PF_INET, SOCK_STREAM, 0);

	error_check(sock_fd);

	error_check(bind(sock_fd, (struct sockaddr *) &my_addr, sizeof(my_addr)));

	error_check(listen(sock_fd, 1));

	client_addr_size = sizeof(client_addr);

	// The extra beackets are because of the == operator in the error_check macro
	error_check((client_sock_fd = accept(sock_fd, (struct sockaddr *) &client_addr, &client_addr_size)));

	error_check(byte_num = recv(client_sock_fd, buff, BUFF_SIZE - 1, 0));

	printf("Recieved: %s\n", buff);

	error_check(send(client_sock_fd, "server response test\n", sizeof("server response test\n"), 0));

	close(sock_fd);
	close(client_sock_fd);

	return 1;
}