#include "server.h"

void prepare_response() {
	char buff[2048] = {0, };

	sprintf(buff, "Heap A: %d\nHeap B: %d\nHeap C: %d\n", heap_a, heap_b, heap_c);

	strcat(response, buff);
}

//this function is to 'decrypt' the client's request
//and perform the client & server move if request is valid.

int client_action(char *request, server_mode *mode) {
	regex_t regex;
	int *curr_heap, remove_num;
	char *end_ptr;
	int regex_res;


	//---------Check if valid command & valid move--------

	if (request[0] == 'Q' && strlen(request) == 1) {
		*mode = STOP;
		return 1;
	}

	//we verify the client's request via regex.
	//the regex is looking for 2 words - a char in the range A-C
	//and a number in the range 0-9999
	error_check(regcomp(&regex, "^[A-C]\\s([0-9]{1,4})$", REG_EXTENDED));

	regex_res = regexec(&regex, request, 0, NULL, 0);
	//printf("%d\n", regex_res);

	//if Invalid command
	if (regex_res == REG_NOMATCH) {
		strcat(response, MOVE_ERR);
		prepare_response();
		strcat(response, CLIENT_TURN);
		regfree(&regex);

		return 0;
	}

	regfree(&regex);

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
		prepare_response();
		strcat(response, CLIENT_TURN);

		regfree(&regex);

		return 0;
	}

	//-----------------------------------------------------


	//-------------valid move - Continue with the game-----------
	//client move
	*curr_heap -= remove_num;

	//check client win
	if (heap_a == 0 && heap_b == 0 && heap_c == 0) {
		prepare_response();
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
		prepare_response();
		strcat(response, WIN_SERVER);
		*mode = STOP;

		return 1;
	}
	//-----------------

	strcat(response, MOVE_OK);
	prepare_response();

	strcat(response, CLIENT_TURN);

	return 1;
	//-----------------------------------------------------
}