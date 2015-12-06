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
		return 0;
	}

	// Send the message to all clients except the sender
	sprintf(response, "Client %d: %s\n", (src_index + 1), (request + 4));

	for (i = 0; i < CLIENT_NUM; i++) {
		// Don't send message back to sender
		if (i != src_index) {
			error_check(send(clients[i], response, strlen(response), 0));
		}
	}

	return 1;
}


/**
* Advance game to next turn, and notify the current client
*/
void moveToNextTurn() {
	client_turn = (client_turn + 1) % CLIENT_NUM;
	
	error_check(send(clients[client_turn], CLIENT_TURN, strlen(CLIENT_TURN), 0));
}


/**
* Notifies all clients that they won/lost the game
* Arguments:
*	int winner_index 	- The winner index
* 	server_mode *mode	- The server mode so it will change to STOP
*/
void clientWon(int winner_index, server_mode *mode) {
	int i;

	error_check(send(clients[winner_index], CLIENT_WIN, strlen(CLIENT_WIN), 0));

	for (i = 0; i < CLIENT_NUM; i++) {
		if (i != winner_index) {
			error_check(send(clients[i], CLIENT_LOSE, strlen(CLIENT_LOSE), 0));
		}
	}

	*mode = STOP;
}

/**
* This function is to 'decrypt' the client's request
* and perform the client & server move if request is valid.
* 
* Argunemts:
* 	char *request 		- The request string from the client
* 	server_mode *mode 	- The current mode of the game
* 	int src_index		- The client number
*/
int client_action(char *request, server_mode *mode, int src_index) {
	regex_t regex;
	int *curr_heap, remove_num;
	char *end_ptr;
	int regex_res;
	int i;

	if (request[0] == 'Q' && strlen(request) == 1) {
		clientWon(((src_index + 1) % CLIENT_NUM ), mode);

		return 1;
	}

	if (checkIfMsg(request, src_index)) {
		return 1;
	}

	if (client_turn != src_index) {
		// Handle out of turn plays
		error_check(send(clients[src_index], OUT_OF_TURN_MSG, strlen(OUT_OF_TURN_MSG), 0));
		return 1;
	}

	//we verify the client's request via regex.
	//the regex is looking for 2 words - a char in the range A-C
	//and a number in the range 0-9999
	error_check(regcomp(&regex, "^[A-C]\\s([0-9]{1,4})$", REG_EXTENDED));

	regex_res = regexec(&regex, request, 0, NULL, 0);
	regfree(&regex);

	// if Invalid command
	if (regex_res == REG_NOMATCH) {
		goto ERROR;
	}

	// Else - Valid command.
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
	
	//----------------Check if valid move----------------
	if (end_ptr == &request[2] || remove_num > 1000 || remove_num <= 0 ||
	 	remove_num > *curr_heap) {

		ERROR:

		// Send error to current user
		sprintf(response, MOVE_ERR);
		error_check(send(clients[src_index], response, strlen(response), 0));

		// Notify other users about error
		sprintf(response, OTHER_CLIENT_MOVE_ERR, (src_index + 1));

		for (i = 0; i < CLIENT_NUM; i++) {
			if (i != src_index) {
				error_check(send(clients[i], response, strlen(response), 0));
			}
		}

		moveToNextTurn();

		return 0;
	}

	//---------------------------------------------------


	//------- valid move - Continue with the game -------
	*curr_heap -= remove_num;

	// Notify client
	bzero(response, BUFF_SIZE);
	sprintf(response, MOVE_OK);
	heapStatuses();

	error_check(send(clients[src_index], response, strlen(response), 0));

	// Notify other clients
	bzero(response, BUFF_SIZE);
	sprintf(response, MOVE_NOTIFY, (src_index + 1), remove_num, request[0]);
	heapStatuses();

	for (i = 0; i < CLIENT_NUM; i++) {
		if (i != src_index) {
			error_check(send(clients[i], response, strlen(response), 0));
		}
	}

	// Check if client won
	if (heap_a == 0 && heap_b == 0 && heap_c == 0) {
		clientWon(src_index, mode);

		return 1;
	}

	moveToNextTurn();

	return 1;
	//---------------------------------------------------
}