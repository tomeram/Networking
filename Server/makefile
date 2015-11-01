all: nim-server

clean:
	rm nim-server nim-server.o

nim-server: nim-server.o
	gcc -o nim-server nim-server.o -Wall -g

nim-server.o: server.h server.c
	gcc -c -o nim-server.o server.c -Wall -g