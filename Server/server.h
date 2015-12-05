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

typedef enum {
	RUN,
	WAITING_CONS,
	STOP
} server_mode;

#endif