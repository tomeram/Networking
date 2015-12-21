#include "tftp-server.h"

// This is an error checking macro.
#define error_check(res) if (res < 0) { 	\
	printf("Error: %s\n", strerror(errno));	\
	exit(1);								\
}

struct sockaddr_in tftp_serveraddr, tftp_clientaddr;
unsigned int clientlen = sizeof(struct sockaddr_in);

int sockfd;
int req_len;

short opcode;
short block;

char request[MAX_BUFF_SIZE]; 
TFTP_PACKET tftp_request;

TFTP_STATUS status;
TFTP_ERROR tftp_error;
TFTP_DATA_BLOCK data_response;

char filename[MAX_DATA_PACKET+1];


/**
* Converts the packet struct to the correct packet string format
*/
void parseInput() {
	short error = 0;
	short op;
	int bytes_read = 0;

	// Clean tftp_request
	tftp_request.opcode = 0;
	tftp_request.block = 0;
	bzero(&(tftp_request.data), sizeof(tftp_request.data));
	bzero(&(tftp_request.mode), sizeof(tftp_request.mode));
	tftp_request.errorCode = 0;

	op = (((short) request[0]) << 8) ^ (short) request[1];

	tftp_request.opcode = op;
	bytes_read += 2;

	switch(op) {
		/*-----------Read/Write Requests--------------
		  2 bytes   string	 1-byte  string  1 byte
		---------------------------------------------
		| opcode | filename | '\0' |  Mode  | '\0' |
		--------------------------------------------*/
		case 1: //read
		case 2: //write
			//error check: request too long or doesn't end in '\0' 
			if (req_len > bytes_read + MAX_DATA_PACKET + 1 || request[req_len] != '\0') {
				error = 1;
				break;
			}

			strncpy(tftp_request.data, (request + bytes_read), MAX_DATA_PACKET + 1);
			
			bytes_read += strlen(tftp_request.data) + 1;

			//error check: filename too long or no 'mode'
			if (req_len <= bytes_read + 1) {
				error = 1;
			}

			strncpy(tftp_request.mode, (request + bytes_read), MAX_DATA_PACKET + 1);
			
			break;
			
		/*-----------Data Packet---------
		   2 bytes   2 bytes	 n bytes
		---------------------------------
		|  opcode  |  block #  |  Data  |
		--------------------------------*/
		case 3: //DATA
			tftp_request.block = (((short) request[bytes_read + 0]) << 8) ^ (short) request[bytes_read + 1];
			bytes_read += 2;

			//error check: request too long...
			if (req_len > bytes_read + MAX_DATA_PACKET) {
				error = 1;
				break;
			}

			strncpy(tftp_request.data, (request + bytes_read), MAX_DATA_PACKET + 1);
			break;

		case 4:
			tftp_request.block = (((short) request[bytes_read + 0]) << 8) ^ (short) request[bytes_read + 1];
			bytes_read += 2;
			break;

		case 5:
			tftp_request.errorCode = (((short) request[bytes_read + 0]) << 8) ^ (short) request[bytes_read + 1];
			bytes_read += 2;
			//error check: request too long...
			if (req_len > bytes_read + MAX_DATA_PACKET || request[req_len] != '\0') {
				error = 1;
				break;
			}
			strncpy(tftp_request.data, (request + bytes_read), MAX_DATA_PACKET + 1);
			break;

		default:
			error = 1;		
			break;
	}

	if (error == 1) {
		tftp_request.opcode = 0;
		status = ERROR;
		tftp_error = ERROR_BAD_REQUEST;
	}
}

/**
* Builds the response string from the TFTP_PACKET struct
*/
int parseOutput(char *response, TFTP_PACKET *msg) {
	// Set Opcode
	response[0] = 0;
	response[1] = msg->opcode;

	switch(msg->opcode) {
		//Data packet
		case 3:

			break;

		// Ack packet
		case 4:
			response[2] = msg->block >> 8;
			response[3] = (msg->block << 8) >> 8;
			break;

		// Error packet
		case 5:
			response[2] = msg->errorCode >> 8;
			response[3] = (msg->errorCode << 8) >> 8;

			strcat(response, msg->data);
			break;

		default:
			break;
	}

	return 1;
}

int sendError(TFTP_PACKET *msg) {
	char response[MAX_DATA_PACKET];

	msg->opcode = 5;
	parseOutput(response, msg);

	return 1;
}

int start_server() {
	int optval;

	/*--Create socket--*/
	sockfd = socket(AF_INET, SOCK_DGRAM,IPPROTO_UDP);
	error_check(sockfd);
	
	/*-------------------------------------------------------------
	taken from google:
	setsockopt: Handy debugging trick that lets us return the server immediately after we kill it;
	otherwise we have to wait about 20 secs.
	eliminates "Error on binding: Address already in use" error.
	*/
	optval = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));
	//-----------------------------------------------------------------


	/* build server adress */
	bzero((char*) &tftp_serveraddr, sizeof(tftp_serveraddr));
	tftp_serveraddr.sin_family = AF_INET;
	tftp_serveraddr.sin_port = htons(PORT_NUM);
	tftp_serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

	/*--Bind Socket--*/
	error_check(bind(sockfd, (struct sockaddr *) &tftp_serveraddr, sizeof(tftp_serveraddr)));

	return 1;
}


//todo change data_response to response buffer
/* returns number of blocks read, -1 for error*/
int read_next_block(int file_fd) {
	int read_bytes,total_bytes;

	bzero(&data_response,sizeof(data_response));
	data_response.opcode = htons(OPCODE_DATA);
	data_response.block_num = htons(block);

	//TODO - handle blocks (currently only sends first 512 chars...? maybe not since file still open - need to check... :) )
	read_bytes = 1;
	total_bytes = 0;

	while (read_bytes > 0 && total_bytes < MAX_DATA_PACKET) {
		read_bytes = read(file_fd, data_response.data + total_bytes, MAX_DATA_PACKET - total_bytes);
		total_bytes += read_bytes;
		//printf("DEBUG: read %d bytes!\n", total_bytes);
	}

	//error check: failed to read from file
	if (read_bytes == -1) {
		return -1;
	}

	return total_bytes;
}

void read_request() {
	int attempts = 0;
	int response_len, isEOF = 0;
	int bytes_read;
	
	int file_fd;
	struct stat file_stat;

	struct sockaddr_in connection_serveraddr;
	int sock_connection;

	//int resend_flag = 0;

	if (stat(filename,&file_stat) == -1 || S_ISDIR(file_stat.st_mode)) {
		status = ERROR;
		tftp_error = ERROR_FILE_NOT_FOUND;
		return;
	}

	//error check: can read from file
	if (access(filename, R_OK) == -1 || (file_fd = open(filename, O_RDONLY)) == -1) {
		status = ERROR;
		tftp_error = ERROR_ACCESS_VIOLATION;
		return;
	}

	//initiate a connection
	if ((sock_connection = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		error_check(close(file_fd));
		printf("Socket Error: %s\n", strerror(errno));
		status = ERROR;
		tftp_error = ERROR_UNDEFINED;
		return;
	}

	bzero(&connection_serveraddr, sizeof(connection_serveraddr));
	connection_serveraddr.sin_family = AF_INET;
	connection_serveraddr.sin_port = htons(0);
	connection_serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if ((bind(sock_connection,(struct sockaddr *) &connection_serveraddr, sizeof(connection_serveraddr))) != 0) {
		error_check(close(file_fd));
		error_check(close(sock_connection));
		printf("Bind Error: %s\n", strerror(errno));
		status = ERROR;
		tftp_error = ERROR_UNDEFINED;
		return;
	}


	block = 1;
	//TODO - timeout

	if ((bytes_read = read_next_block(file_fd)) == -1) {
		printf("read from file Error: %s\n", strerror(errno));
		error_check(close(file_fd));
		error_check(close(sock_connection));
		status = ERROR;
		tftp_error = ERROR_UNDEFINED; //TODO is it error undefined or access violation?..
	}
	response_len = bytes_read + sizeof(opcode) + sizeof(block);

	//todo change data_response to response buffer
	if (sendto(sock_connection, (char *)&data_response, response_len, 0, (struct sockaddr*)&tftp_clientaddr, clientlen) == -1) {
		printf("send Error: %s\n", strerror(errno));
		error_check(close(file_fd));
		error_check(close(sock_connection));
		status = ERROR;
		tftp_error = ERROR_UNDEFINED;
		return;
	}

	if (bytes_read < MAX_DATA_PACKET) {
		isEOF = 1;
	}

	//TODO - send errors for wrong/bad packets? or try few times before sending error?
	while(1) {
		if (attempts == MAX_BAD_ATTEMPTS) {
			error_check(close(file_fd));
			error_check(close(sock_connection));
			status = ERROR;
			tftp_error = ERROR_BAD_ATTEMPTS;
			return;  
		}

		//TODO - get ACK
		req_len = recvfrom(sock_connection, &request, sizeof(request), 0, (struct sockaddr *) &tftp_clientaddr, &clientlen);

		if (req_len == -1) {
			printf("send Error: %s\n", strerror(errno));
			error_check(close(file_fd));
			error_check(close(sock_connection));
			status = ERROR;
			tftp_error = ERROR_UNDEFINED;
			return;
		}

		if (req_len < sizeof(opcode)) { //Error: Header too small
			attempts ++;
			continue;
		}

		parseInput();

		if (status == ERROR) {
			attempts ++;
			status = OK;
			continue;
		}

		//error check 2: opcode is ACK
		if (tftp_request.opcode != OPCODE_ACK) {
			attempts ++;
			continue;
		}

		printf("ack for block: %d\n",tftp_request.block);



		if (tftp_request.block == block) { 

			if (isEOF) {
				break;
			}

			block++;
			if ((bytes_read = read_next_block(file_fd)) == -1) {
				printf("read from file Error: %s\n", strerror(errno));
				error_check(close(file_fd));
				error_check(close(sock_connection));
				status = ERROR;
				tftp_error = ERROR_UNDEFINED; //TODO is it error undefined or access violation?..
			}
			response_len = bytes_read + sizeof(opcode) + sizeof(block);

			if (bytes_read < MAX_DATA_PACKET) {
				isEOF = 1;
			}
		}

		//todo change data_response to response buffer
		//if ack for prev block, or ack for curr block - send again/next block
		if (tftp_request.block == block - 2 || tftp_request.block == block - 1) {
			if (sendto(sock_connection, (char *)&data_response, response_len, 0, (struct sockaddr*)&tftp_clientaddr, clientlen) == -1) {
				printf("send Error: %s\n", strerror(errno));
				error_check(close(file_fd));
				error_check(close(sock_connection));
				status = ERROR;
				tftp_error = ERROR_UNDEFINED;
				return;
			}
		}
	}

	error_check(close(file_fd));
	error_check(close(sock_connection));
}

void write_request() {
	int file_fd;
	TFTP_PACKET res_data;
	res_data.block = 0;

	int sock_connection;
	struct sockaddr_in connection_serveraddr;

	char response[MAX_BUFF_SIZE] = {0, };

	printf("Write Request\n");

	// File exists
	if (access(filename, F_OK) != -1) {
		status = ERROR;
		tftp_error = ERROR_FILE_EXISTS;
		return;
	}

	file_fd = open(filename, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

	// Open connection with new random port (TID)
	if ((sock_connection = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		error_check(close(file_fd));
		printf("Socket Error: %s\n", strerror(errno));
		status = ERROR;
		tftp_error = ERROR_UNDEFINED;
		return;
	}

	bzero(&connection_serveraddr, sizeof(connection_serveraddr));
	connection_serveraddr.sin_family = AF_INET;
	connection_serveraddr.sin_port = htons(0);
	connection_serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if ((bind(sock_connection,(struct sockaddr *) &connection_serveraddr, sizeof(connection_serveraddr))) != 0) {
		error_check(close(file_fd));
		error_check(close(sock_connection));
		printf("Bind Error: %s\n", strerror(errno));
		status = ERROR;
		tftp_error = ERROR_UNDEFINED;
		return;
	}

	res_data.opcode = 4;
	parseOutput(response, &res_data);

	if (sendto(sock_connection, response, 4, 0, (struct sockaddr*)&tftp_clientaddr, clientlen) == -1) {
		printf("send Error: %s\n", strerror(errno));
		error_check(close(file_fd));
		error_check(close(sock_connection));
		status = ERROR;
		tftp_error = ERROR_UNDEFINED;
		return;
	}

	while(1) {
		// Recive packet and write to file
		bzero(&request, sizeof(request)); 
		req_len = recvfrom(sock_connection, &request, sizeof(request), 0, (struct sockaddr *) &tftp_clientaddr, &clientlen);
		parseInput();

		// write to file
		res_data.block++;
		printf("op: %d, block: %d, data: %s\n\n", tftp_request.opcode, tftp_request.block, tftp_request.data);

		if (write(file_fd, tftp_request.data, strlen(tftp_request.data)) < 0) {
			printf("write Error: %s\n", strerror(errno));

			status = ERROR;
			error_check(close(file_fd));
			error_check(close(sock_connection));

			if (errno == ENOSPC) {
				tftp_error = ERROR_DISK_FULL;
			} else {
				tftp_error = ERROR_UNDEFINED;
			}

			return;
		}

		// Send ack
		bzero(response, MAX_BUFF_SIZE);
		res_data.opcode = 4;
		parseOutput(response, &res_data);

		if (sendto(sock_connection, response, 4, 0, (struct sockaddr*)&tftp_clientaddr, clientlen) == -1) {
			printf("send Error: %s\n", strerror(errno));
			error_check(close(file_fd));
			error_check(close(sock_connection));
			status = ERROR;
			tftp_error = ERROR_UNDEFINED;
			return;
		}

		// If done, break
		if (strlen(tftp_request.data) < MAX_DATA_PACKET) {
			break;
		}
	}

	error_check(close(file_fd));
	error_check(close(sock_connection));
}

void new_request() {
	int i;

	if (req_len < sizeof(opcode)) { //Error: Header too small
		status = ERROR;
		tftp_error = ERROR_UNDEFINED;  
		return;
	}

	parseInput();

	if (status == ERROR) {
		return;
	}

	//error check 2: opcode is read/write
	if (tftp_request.opcode != OPCODE_READ && tftp_request.opcode != OPCODE_WRITE) {
		status = ERROR;
		tftp_error = ERROR_ILLEGAL_OP;
		return;
	}

	bzero(filename, MAX_DATA_PACKET+1);

	strcpy(filename,tftp_request.data);
	
	printf("filename: %s\nmode: %s\nopcode: %d\n", filename,tftp_request.mode,tftp_request.opcode);

	//error check 4: mode == 'octet'
	for (i = 0 ; tftp_request.mode[i]; i++) {
		tftp_request.mode[i] = tolower(tftp_request.mode[i]);
	}

	if (strcmp(tftp_request.mode, "octet") != 0) {
		status = ERROR;
		tftp_error = ERROR_WRONG_MODE;
		return;
	}

	if (tftp_request.opcode == OPCODE_READ) {
		read_request();
	}

	if (tftp_request.opcode == OPCODE_WRITE) {
		write_request();
	}
 
	return;
}

int main(int argc, char **argv) {
	if (argc != 1) {
		printf("Too many arguments given\n");
		return 0;
	}

	if (start_server() != 1) {
		return 0;
	}

	while (1) {
		bzero(&request, sizeof(request)); 
		req_len = recvfrom(sockfd, &request, sizeof(request), 0, (struct sockaddr *) &tftp_clientaddr, &clientlen);
		
		if (req_len < 0) {
			continue;
		}

		new_request();

		if (status == ERROR) {
			printf("error!\n");
			status = OK; //bypass till to do
		}

	}

	close(sockfd);
}

