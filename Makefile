all: client server

client: client.o
	gcc -pthread -o client client.o

client.o: client.c
	gcc -c client.c

server: server.o
	gcc -pthread -o server server.o 

server.o: server.c
	gcc -c server.c


clean:
	rm *.o client server