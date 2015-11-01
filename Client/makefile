all: nim

clean:
	rm nim nim.o

nim: nim.o
	gcc -o nim nim.o -Wall -g

nim.o: client.h client.c
	gcc -c -o nim.o client.c -Wall -g