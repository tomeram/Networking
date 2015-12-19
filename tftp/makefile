all: tftp-server

clean:
	rm tftp-server tftp-server.o

tftp-server: tftp-server.o
	gcc -o tftp-server tftp-server.o -Wall -g

tftp-server.o: tftp-server.h tftp-server.c
	gcc -c -o tftp-server.o tftp-server.c -Wall -g