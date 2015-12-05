all: nim-server

clean:
	rm nim-server nim-server.o nim-server-funcs.o

nim-server: nim-server.o nim-server-funcs.o
	gcc -o nim-server nim-server.o nim-server-funcs.o -Wall -g

nim-server.o: server.h server.c
	gcc -c -o nim-server.o server.c -Wall -g

nim-server-funcs.o: server.h nim-server-funcs.c
	gcc -c -o nim-server-funcs.o nim-server-funcs.c -Wall -g