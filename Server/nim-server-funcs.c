#include "server.h"

/**
* Concatinates the heap status string to the response buffer
*/
void heapStatuses() {
	char buff[2048] = {0, };

	sprintf(buff, "Heap A: %d\nHeap B: %d\nHeap C: %d\n", heap_a, heap_b, heap_c);

	strcat(response, buff);
}


/**
* Cehcks if the request is a properly formatted message,
* and send it to all the clients
*/
int checkIfMsg(char *request, int src_index) {
	regex_t regex;
	int regex_res;
	char response[2048] = {0, };
	int i;

	error_check(regcomp(&regex, "^(MSG)\\s.*$", REG_EXTENDED));
	regex_res = regexec(&regex, request, 0, NULL, 0);
	regfree(&regex);

	if (regex_res == REG_NOMATCH) {
		// Not a message
		printf("Here\n");
		return 0;
	}

	// Send the message to all clients except the sender
	sprintf(response, "Client %d: %s\n", (src_index + 1), (request + 4));
	printf("Sent message:\n%s\n", response);

	for (i = 0; i < CLIENT_NUM; i++) {
		// Don't send message back to sender
		if (i != src_index) {
			error_check(send(clients[i], response, strlen(response), 0));
		}
	}

	return 1;
}


/**
* This function is to 'decrypt' the client's request
* and perform the client & server move if request is valid.
*/
int client_action(char *request, server_mode *mode, int src_index) {
	regex_t regex;
	int *curr_heap, remove_num;
	char *end_ptr;
	int regex_res;

	//---------Check if valid command & valid move--------

	if (request[0] == 'Q' && strlen(request) == 1) {
		*mode = STOP;
		return 1;
	}

	if (checkIfMsg(request, src_index)) {
		return 1;
	}

	//we verify the client's request via regex.
	//the regex is looking for 2 words - a char in the range A-C
	//and a number in the range 0-9999
	error_check(regcomp(&regex, "^[A-C]\\s([0-9]{1,4})$", REG_EXTENDED));

	regex_res = regexec(&regex, request, 0, NULL, 0);
	regfree(&regex);

	//if Invalid command
	if (regex_res == REG_NOMATCH) {
		strcat(response, MOVE_ERR);
		heapStatuses();
		strcat(response, CLIENT_TURN);

		return 0;
	}

	//Else - Valid command.
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
	
	//----Check if valid move----
	if (end_ptr == &request[2] || remove_num > 1000 || remove_num <= 0 ||
	 	remove_num > *curr_heap) {
		strcat(response, MOVE_ERR);
		heapStatuses();
		strcat(response, CLIENT_TURN);


		return 0;
	}

	//-----------------------------------------------------


	//-------------valid move - Continue with the game-----------
	//client move
	*curr_heap -= remove_num;

	//check client win
	if (heap_a == 0 && heap_b == 0 && heap_c == 0) {
		heapStatuses();
		strcat(response, CLIENT_WIN);
		*mode = STOP;

		return 1;
	}
	//-----------------

	//Computer's Move
	curr_heap = &heap_a;

	if (heap_b > heap_a) {
		curr_heap = &heap_b;
	} 
	if (*curr_heap < heap_c) {
		curr_heap = &heap_c;
	}

	*curr_heap -= 1;

	//check computer win
	if (heap_a == 0 && heap_b == 0 && heap_c == 0) {
		heapStatuses();
		strcat(response, WIN_SERVER);
		*mode = STOP;

		return 1;
	}
	//-----------------

	strcat(response, MOVE_OK);
	heapStatuses();

	strcat(response, CLIENT_TURN);

	return 1;
	//-----------------------------------------------------
}