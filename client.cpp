#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include "packet.cpp"

#define PORT 4000
#define BUFFER_SIZE 256
#define QUIT 1
#define SEND 2
#define FOLLOW 3
#define TRUE 1

int sockfd;
int seqncnt = 0;

int main(int argc, char *argv[])
{
	pthread_t thr_client_input, thr_client_display;
	
    int n;
	unsigned int length;
	struct sockaddr_in serv_addr, from;
	struct hostent *server;
	
	char buffer[256];
	if (argc < 2) {
		fprintf(stderr, "usage %s hostname\n", argv[0]);
		exit(0);

	}
	
	server = gethostbyname(argv[1]);
	if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }	
	
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		printf("ERROR opening socket");
	
	serv_addr.sin_family = AF_INET;     
	serv_addr.sin_port = htons(PORT);    
	serv_addr.sin_addr = *((struct in_addr *)server->h_addr);
	bzero(&(serv_addr.sin_zero), 8);  

	printf("Enter the message: ");
	bzero(buffer, 256);
	fgets(buffer, 256, stdin);

	n = sendto(sockfd, buffer, strlen(buffer), 0, (const struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in));
	if (n < 0) 
		printf("ERROR sendto");
	
	length = sizeof(struct sockaddr_in);
	n = recvfrom(sockfd, buffer, 256, 0, (struct sockaddr *) &from, &length);
	if (n < 0)
		printf("ERROR recvfrom");

	printf("Got an ack: %s\n", buffer);
	
	close(sockfd);
	return 0;
}

//function to send messages from client
//should be called in a separate thread 
//as to not conflict with receiving messages
void *sendmessage(void *arg){
	int n, flag, action, sockfd = *(int *) arg; 
	char buffer[BUFFER_SIZE];

	while(1)
	{
		//clears buffer
		bzero(buffer, BUFFER_SIZE);
		if(!fgets(buffer, BUFFER_SIZE, stdin))
			action = QUIT;
		else
			action = getaction(buffer);
		switch(action){
			case QUIT:
			//isso aqui vai ser o handle de sair da sessão
				closeSession;
				break;
			case SEND:
				if (strlen(buffer) > 134){
					printf("Message exceeds character limit (128)\n");
				}
				else{
					sendpacket(sockfd, SEND, ++seqncnt, strlen(buffer)-5, getcurrenttime(), buffer+5*sizeof(char));
				}
				break;
			case FOLLOW:
				sendpacket(sockfd, FOLLOW, ++seqncnt,  strlen(buffer)-7, getcurrenttime(), buffer+7*sizeof(char));
				break;
			default:
				printf("Action unknown. Should be:\nSEND <message>\nFOLLOW <@user>\nQUIT");	
		}
	}
}


int getaction(char* buffer){
	if(!strncmp(buffer, "SEND ", 5))
		return SEND;

	if(!strncmp(buffer, "FOLLOW ", 7))
		return FOLLOW;
//se não for follow ou send retorna erro (-1)
	else
		return -1;
}
//fecha a session quando der ctrl c no terminal
void closeSession(){
	sendpacket(sockfd,QUIT,++seqncnt,0,0,"");
    recvprintpacket(sockfd);
    close(sockfd);
    system("clear"); 
    printf("Session closed\n");
    exit(1);
}
//pega o tempo no momento da chamada
int getcurrenttime(){
	time_t currenttime = time(NULL);
	struct tm *structtm = localtime(&currenttime);
	int hr = (*structtm).tm_hour;
	int min = (*structtm).tm_min;

	return hr*100+min;
}
