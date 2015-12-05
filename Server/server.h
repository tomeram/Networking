#ifndef _SERVER_
#define _SERVER_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <regex.h>

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
#define REJECT_CON "Client rejected"
#define CLIENT1_CONNECT "You are client 1\nWaiting to client 2 to connect\n"
#define CLIENT2_CONNECT "You are client 2\n"
#define TRUE 1

typedef enum {
	STOP,
	WAITING_CONS,
	CLIENT_1,
	CLIENT_2,
	RUN
} server_mode;

extern int heap_a, heap_b, heap_c;
extern server_mode mode;
extern char response[BUFF_SIZE];

// Functions
void prepare_response();
int client_action(char *request, server_mode *mode);

#endif