all: client server testserver testclient

client: client.o
	gcc -pthread -o client client.o

client.o: client.c
	gcc -c client.c

server: server.o
	gcc -pthread -o server server.o 

server.o: server.c
	gcc -c server.c

testserver: testserver.o
	gcc -pthread -o testserver testserver.o 

testserver.o: testserver.c
	gcc -c testserver.c

testclient: testclient.o
	gcc -pthread -o testclient testclient.o 

testclient.o: testclient.c
	gcc -c testclient.c

clean:
	rm *.o client server testserver testclient