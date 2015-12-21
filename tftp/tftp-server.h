#ifndef _TFTP_
#define _TFTP_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <assert.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>

#define PORT_NUM 6900
#define MAX_DATA_PACKET 512

//longest message possible - Error: opcode(2b) + errorcode(2b) + string + '\0'
#define MAX_BUFF_SIZE (2 + 2 + MAX_DATA_PACKET + 1)

#define TIMEOUT_INTERVAL 10 //10 secs 

typedef enum {
	OK,
	READ,
	WRITE,
	ERROR
} TFTP_STATUS;

typedef enum {
	OPCODE_READ = 1,
	OPCODE_WRITE = 2,
	OPCODE_DATA = 3,
	OPCODE_ACK = 4,
	OPCODE_ERROR = 5
} TFTP_CODE;

typedef enum {
	ERROR_UNDEFINED = 0,
	ERROR_FILE_NOT_FOUND = 1,
	ERROR_ACCESS_VIOLATION = 2,
	ERROR_DISK_FULL = 3,
	ERROR_ILLEGAL_OP = 4,
	ERROR_UNKNOWN_ID = 5,
	ERROR_FILE_EXISTS = 6,
	ERROR_NO_USER = 7, 
	//our additions
	ERROR_BAD_REQUEST = 8,
	ERROR_WRONG_MODE = 9,
	ERROR_BAD_ATTEMPTS = 10
} TFTP_ERROR;


typedef struct {
	short opcode;
	short block;
	char data[MAX_DATA_PACKET + 1];
	char mode[MAX_DATA_PACKET + 1];
	short errorCode;
} TFTP_PACKET;

typedef struct {
	short opcode;
	char data[MAX_DATA_PACKET + 1];
} TFTP_MSG;

typedef struct {
	short opcode;
	short block_num;
	char data[MAX_DATA_PACKET + 1];
} TFTP_DATA_BLOCK;



#endif

