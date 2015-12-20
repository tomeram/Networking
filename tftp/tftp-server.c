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

TFTP_MSG request;
TFTP_STATUS status;
TFTP_ERROR tftp_error;
TFTP_DATA_BLOCK data_response;

char filename[MAX_DATA_PACKET+1];
char mode[MAX_DATA_PACKET+1];

int file_fd;

struct sockaddr_in connection_serveraddr, connection_clientaddr;
int sock_connection;

short block;

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


void read_request() {
	struct stat file_stat;
	int response_len,read_bytes,total_bytes;
	//int resend_flag = 0;

	if (status != READ) { //first request for read
		//error check: file exists & not a dir
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

		status = READ;
		block = 0;

		//TODO - timeout
	}

	else {
		//TODO - get ACK

		//if not
		//resend_flag = 1;
		//TODO - check if final ACK - if so - close socket
	}
	
	/*-----------Data Packet---------
	   2 bytes   2 bytes	 n bytes
	---------------------------------
	|  opcode  |  block #  |  Data  |
	--------------------------------*/
	/*if (resend_flag) {
		if (sendto(sock_connection, &data_response, response_len, 0, (struct sockaddr*)&tftp_clientaddr, clientlen) == -1) {
			printf("send Error: %s\n", strerror(errno));
			error_check(close(file_fd));
			error_check(close(sock_connection));
			status = ERROR;
			tftp_error = ERROR_UNDEFINED;
			return;
		}
	}*/

	bzero(&data_response,sizeof(data_response));
	data_response.opcode = htons(OPCODE_DATA);
	data_response.block_num = htons(block);

	//TODO - handle blocks (currently only sends first 512 chars...? maybe not since file still open - need to check... :) )
	read_bytes = 1;
	total_bytes = 0;
	while (read_bytes > 0 && total_bytes < MAX_DATA_PACKET) {
		read_bytes = read(file_fd, data_response.data + total_bytes, MAX_DATA_PACKET - total_bytes);
		total_bytes += read_bytes;
		printf("DEBUG: read %d bytes!\n", total_bytes);
	}

	//error check: failed to read from file
	if (read_bytes == -1) {
		printf("read from file Error: %s\n", strerror(errno));
		error_check(close(file_fd));
		error_check(close(sock_connection));
		status = ERROR;
		tftp_error = ERROR_UNDEFINED; //TODO is it error undefined or access violation?..
		return;
	}

	response_len = sizeof(opcode) + sizeof(block) + total_bytes;
	if (sendto(sock_connection, &data_response, response_len, 0, (struct sockaddr*)&tftp_clientaddr, clientlen) == -1) {
		printf("send Error: %s\n", strerror(errno));
		error_check(close(file_fd));
		error_check(close(sock_connection));
		status = ERROR;
		tftp_error = ERROR_UNDEFINED;
		return;
	}

	printf("sent?\n");

	if (total_bytes < MAX_DATA_PACKET) {
		close(file_fd);
	}

}

void write_request() {

}


/*-----------Read/Write Requests--------------
  2 bytes   string	 1-byte  string  1 byte
---------------------------------------------
| opcode | filename | '\0' |  Mode  | '\0' |
--------------------------------------------*/
void new_request() {
	int i;

	bzero(filename, MAX_DATA_PACKET+1);
	bzero(mode,MAX_DATA_PACKET+1);

	printf("DEBUG: new request!\n");

	//error check 1: we got the full "request"
	if (request.data[req_len - sizeof(opcode) -1] != '\0') {
		status = ERROR;
		tftp_error = ERROR_BAD_REQUEST;
		return;
	}

	//error check 2: opcode is read/write
	if (request.opcode != OPCODE_READ && request.opcode != OPCODE_WRITE) {
		status = ERROR;
		tftp_error = ERROR_ILLEGAL_OP;
		return;
	}

	strcpy(filename,request.data);

	//error check 3: missing '\0' after filename
	if (strlen(filename)+sizeof(opcode)+1 == req_len) {
		status = ERROR;
		tftp_error = ERROR_BAD_REQUEST;
		return;
	}

	strcpy(mode,request.data + strlen(filename) +1);
	
	//printf("filename: %s\nmode: %s\nopcode: %d\n", filename,mode,request.opcode);

	//error check 4: mode == 'octet'
	for (i = 0 ; mode[i]; i++) {
		mode[i] = tolower(mode[i]);
	}
	if (strcmp(mode, "octet") != 0) {
		status = ERROR;
		tftp_error = ERROR_WRONG_MODE;
		return;
	}

	if (request.opcode == OPCODE_READ) {
		return read_request();
	}

	if (request.opcode == OPCODE_WRITE) {
		return write_request();
	}
 
	return;
}

void handle_request() {
	if (req_len < sizeof(opcode)) { //Error: Header too small
		status = ERROR;
		tftp_error = ERROR_UNDEFINED;  
		return;
	}

	request.opcode = ntohs(request.opcode); //TODO <--check if needed?... seems to conver network to short?

	if (request.opcode == OPCODE_ERROR) {
		//TODO
		return;
	}

	switch (status) {
		case OK: //server idle
			new_request();
			break;

		case READ: //server in mid-read
			read_request();
			break;

		case WRITE: //server in mid-write
			write_request();
			break;

		case ERROR:
			printf("todo- error");
			//TODO not sure if needs to do something, this is here cause of gcc warning :)
			break;
	}	

}

int main(int argc, char **argv) {
	if (argc != 1) {
		printf("Too many arguments given\n");
		return 0;
	}

	if (start_server() != 1) {
		return 0;
	}

	while (1) { //TODO: timeouts...

		bzero(&request, sizeof(request)); 
		req_len = recvfrom(sockfd, &request, sizeof(TFTP_MSG), 0, (struct sockaddr *) &tftp_clientaddr, &clientlen);
		
		if (req_len != -1) {
			handle_request();
		}

		if (status == ERROR) {
			//TODO - send error
		}

	}

	close(sockfd);
	//freeaddrinfo(&serveraddr);
	//freeaddrinfo(&clientaddr);
}

