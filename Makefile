all: client server

client: client.o
	gcc -pthread -o client client.o

client.o: client.cpp
	gcc -c client.cpp

server: server.o
	gcc -pthread -o server server.o 

server.o: server.cpp
	gcc -c server.cpp


clean:
	rm *.o client server