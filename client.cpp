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
#include <pthread.h>
#include "packet.cpp"

#define PORT 4000
#define BUFFER_SIZE 256
#define QUIT 1
#define SEND 2
#define FOLLOW 3
#define NOTIFICATION 4
#define SERVERMSG 5
#define LOGUSER 6
#define TRUE 1

int sockfd;
int seqncnt = 0;

int main(int argc, char *argv[])
{
	pthread_t thr_client_input, thr_client_display;
	
    int n, port;
	unsigned int length;
	char handle[20];
	struct sockaddr_in serv_addr, from;
	struct hostent *server;
	
	char buffer[256];
	if (argc < 4) {
		fprintf(stderr, "usage ./app_cliente <@profile> <server_address> <port> \n");
		exit(0);

	}
	
	server = gethostbyname(argv[2]);
	if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }

	port = atoi(argv[3]);

	strcpy(handle, argv[1]);	
	validateuserhandle(handle);
	printf("User handle: %s , Port: %d\nUse SEND to send a message to all followers.\nUse FOLLOW <@handle> to follow another user.",handle, port);

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		printf("ERROR opening socket");
	
	serv_addr.sin_family = AF_INET;     
	serv_addr.sin_port = htons(port);    
	serv_addr.sin_addr = *((struct in_addr *)server->h_addr);
	bzero(&(serv_addr.sin_zero), 8);  

	if((connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr))<0)){
		printf("Socket connection error\n");
	}

	sendhandletoserver(handle);


//Começa uma thread para receber mensagens e outra para enviar
	pthread_create(&thr_client_display, NULL, receivemessage, &sockfd);
	pthread_create(&thr_client_input, NULL, sendmessage, &sockfd);
	
	pthread_join(thr_client_display, NULL);
	pthread_join(thr_client_input, NULL);
	close(sockfd);
	return 0;
}

//function to send messages from client
//should be called in a separate thread 
//as to not conflict with receiving messages
void *sendmessage(void *arg){
	int n, flag, action, sockfd = *(int *) arg; 
	char buffer[BUFFER_SIZE];

	while(TRUE)
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

void *receivemessage(void *arg){
	int sockfd = *(int *) arg;
	packet msg;

	while(TRUE){
		recvpacket(sockfd, &msg);
		switch(msg.type){
			case FOLLOW:
				printf("%s\n", msg._payload);
				break;
			case NOTIFICATION:
				printf("%s\n", msg._payload);
				break;
			case QUIT:
				printf ("Limite de sessões simultâneas excedido.");
				close (sockfd);
				exit(1);
				break;
			default:
				printf ("Unknown message error");
				break;
		}
		free(msg._payload);
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

void validateuserhandle(char *handle){
	//Testa o comprimento do username
	if (!(strlen(handle) <= 20 && strlen(handle)>=4)){
		printf("Username must have between 4 and 20 characters.");
		exit(0);
	}
	//Testa se o primeiro caractere é @
	if((handle[0]!='@')){
		printf("Handle must start with @\nExample: @TestUser");
	}
}

void sendhandletoserver(char *handle){
	sendpacket(sockfd, LOGUSER, ++seqncnt, strlen(handle)+1, getcurrenttime(), handle);
}