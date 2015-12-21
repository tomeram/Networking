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

TFTP_PACKET error_packet;

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
		bzero(error_packet.data,sizeof(error_packet.data));
		error_packet.errorCode = ERROR_UNDEFINED;
		strcat(error_packet.data, "Bad Request");
		tftp_request.opcode = 0;
		status = ERROR;
		tftp_error = ERROR_UNDEFINED;
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

			strcat(&response[4], msg->data);
			break;

		default:
			break;
	}

	return 1;
}

void sendError(TFTP_PACKET *msg, int sockfd) {
	char response[MAX_DATA_PACKET] = {0, };

	msg->opcode = 5;
	parseOutput(response, msg);

	sendto(sockfd, response, strlen(msg->data) + 4, 0, (struct sockaddr*)&tftp_clientaddr, clientlen);
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
	int response_len, isEOF = 0;
	int bytes_read;
	
	int file_fd;
	struct stat file_stat;

	struct sockaddr_in connection_serveraddr;
	int sock_connection;

	struct timeval tv;

	TFTP_PACKET res_data;

	//initiate a connection
	if ((sock_connection = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
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
		error_check(close(sock_connection));
		printf("Bind Error: %s\n", strerror(errno));
		status = ERROR;
		tftp_error = ERROR_UNDEFINED;
		return;
	}

	tv.tv_sec = TIMEOUT_INTERVAL;
	tv.tv_usec = 0;
	if ((setsockopt(sock_connection,SOL_SOCKET,SO_RCVTIMEO, &tv, sizeof(tv))) == -1) {
		error_check(close(sock_connection));
		printf("Setsockopt (Timeout) Error: %s\n", strerror(errno));
		status = ERROR;
		tftp_error = ERROR_UNDEFINED;
		return;
	}

	if (stat(filename,&file_stat) == -1 || S_ISDIR(file_stat.st_mode)) {
		bzero(res_data.data,sizeof(res_data.data));
		res_data.errorCode = ERROR_FILE_NOT_FOUND;
		strcat(res_data.data, "File not found");

		sendError(&res_data, sock_connection);
		error_check(close(sock_connection));
		return;
	}

	//error check: can read from file
	if (access(filename, R_OK) == -1 || (file_fd = open(filename, O_RDONLY)) == -1) {
		bzero(res_data.data,sizeof(res_data.data));
		res_data.errorCode = ERROR_ACCESS_VIOLATION;
		strcat(res_data.data, "Cannot open file");

		sendError(&res_data, sock_connection);
		error_check(close(sock_connection));
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

	while(1) {
		req_len = recvfrom(sock_connection, &request, sizeof(request), 0, (struct sockaddr *) &tftp_clientaddr, &clientlen);
		if (req_len == -1) {
			break; //timeout - or error in recvfrom
		}

		if (req_len < sizeof(opcode)) { //Error: Header too small
			bzero(res_data.data,sizeof(res_data.data));
			res_data.errorCode = ERROR_UNDEFINED;
			strcat(res_data.data, "No opcode found (BAD REQUEST)");

			sendError(&res_data, sock_connection);

			continue;
		}

		parseInput();

		if (status == ERROR) {
			bzero(res_data.data,sizeof(res_data.data));
			res_data.errorCode = ERROR_UNDEFINED;
			strcat(res_data.data, "Bad request");

			sendError(&res_data, sock_connection);

			status = OK;
			continue;
		}

		//error check 2: opcode is ACK
		if (tftp_request.opcode != OPCODE_ACK) {
			bzero(res_data.data,sizeof(res_data.data));
			res_data.errorCode = ERROR_ILLEGAL_OP;
			strcat(res_data.data, "Expected Ack opcode");

			sendError(&res_data, sock_connection);

			status = OK;
			continue;
		}

		if (tftp_request.block == block) { 

			if (isEOF) {
				break;
			}

			block++;
			if ((bytes_read = read_next_block(file_fd)) == -1) {
				printf("read from file Error: %s\n", strerror(errno));
				bzero(res_data.data,sizeof(res_data.data));
				res_data.errorCode = ERROR_ACCESS_VIOLATION;
				strcat(res_data.data, "Failed to read from file");

				sendError(&res_data, sock_connection);
				break;
			}
			response_len = bytes_read + sizeof(opcode) + sizeof(block);

			if (bytes_read < MAX_DATA_PACKET) {
				isEOF = 1;
			}
		}

		//if ack for prev block, or ack for curr block - send again/next block
		if (tftp_request.block == block - 2 || tftp_request.block == block - 1) {
			if (sendto(sock_connection, (char *)&data_response, response_len, 0, (struct sockaddr*)&tftp_clientaddr, clientlen) == -1) {
				printf("send Error: %s\n", strerror(errno));
				break;
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

	// Open connection with new random port (TID)
	if ((sock_connection = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
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
		error_check(close(sock_connection));
		printf("Bind Error: %s\n", strerror(errno));
		status = ERROR;
		tftp_error = ERROR_UNDEFINED;
		return;
	}

	// Check if ile exists
	if (access(filename, F_OK) != -1) {
		printf("File already exists error\n");
		res_data.errorCode = ERROR_FILE_EXISTS;
		strcat(res_data.data, "File already exists on server");

		sendError(&res_data, sock_connection);

		return;
	}

	file_fd = open(filename, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

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

		// Wrong opcode recieved
		if (tftp_request.opcode != OPCODE_DATA) {
			printf("File already exists error\n");
			res_data.errorCode = ERROR_ILLEGAL_OP;
			strcat(res_data.data, "Expected data op code");

			sendError(&res_data, sock_connection);

			continue;
		}

		// An ack packet wasn't recieved, re-transmit it
		if (tftp_request.block == res_data.block) {
			goto ACK_SEND;
		}

		// write to file
		res_data.block++;

		if (write(file_fd, tftp_request.data, strlen(tftp_request.data)) < 0) {
			printf("write Error: %s\n", strerror(errno));

			status = ERROR;

			if (errno == ENOSPC) {
				res_data.errorCode = ERROR_DISK_FULL;
				strcat(res_data.data, "No space left on server");
			} else {
				res_data.errorCode = ERROR_UNDEFINED;
				strcat(res_data.data, "An unknown error has occured");
			}

			sendError(&res_data, sock_connection);

			break;
		}

		ACK_SEND:

		// Send ack
		bzero(response, MAX_BUFF_SIZE);
		res_data.opcode = 4;
		parseOutput(response, &res_data);

		if (sendto(sock_connection, response, 4, 0, (struct sockaddr*)&tftp_clientaddr, clientlen) == -1) {
			printf("send Error: %s\n", strerror(errno));;
			status = ERROR;
			tftp_error = ERROR_UNDEFINED;
			break;
		}

		// If done, break
		if (strlen(tftp_request.data) < MAX_DATA_PACKET) {
			break;
		}
	}

	error_check(close(file_fd));
	error_check(close(sock_connection));

	if (status == ERROR) {
		remove(filename);
	}
}

void new_request() {
	int i;

	if (req_len < sizeof(opcode)) { //Error: Header too small
		bzero(error_packet.data,sizeof(error_packet.data));
		error_packet.errorCode = ERROR_UNDEFINED;
		strcat(error_packet.data, "Bad Request (no opcode found)");
		
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
		bzero(error_packet.data,sizeof(error_packet.data));
		error_packet.errorCode = ERROR_ILLEGAL_OP;
		strcat(error_packet.data, "Expected Read/Write opcode");

		status = ERROR;
		tftp_error = ERROR_ILLEGAL_OP;
		return;
	}

	bzero(filename, MAX_DATA_PACKET+1);

	strcpy(filename,tftp_request.data);
	
	//printf("filename: %s\nmode: %s\nopcode: %d\n", filename,tftp_request.mode,tftp_request.opcode);

	//error check 4: mode == 'octet'
	for (i = 0 ; tftp_request.mode[i]; i++) {
		tftp_request.mode[i] = tolower(tftp_request.mode[i]);
	}

	if (strcmp(tftp_request.mode, "octet") != 0) {
		bzero(error_packet.data,sizeof(error_packet.data));
		error_packet.errorCode = ERROR_UNDEFINED;
		strcat(error_packet.data, "wrong mode (expected octet)");

		status = ERROR;
		tftp_error = ERROR_UNDEFINED;
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
			sendError(&error_packet, sockfd);
			status = OK;
		}

	}

	close(sockfd);
}

