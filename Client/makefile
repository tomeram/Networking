all: nim

clean:
	rm nim nim.o

nim: nim.o
	gcc -o nim nim.o

nim.o: client.h client.c
	gcc -c -o nim.o client.c