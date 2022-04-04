#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <signal.h>
#include <pthread.h>
#include "commons.h"
#include "packet.cpp"

#define PORT 4000
#define SIGINT 2
int sockfd, n;
struct sockaddr_in serv_addr;

pthread_mutex_t send_mutex =  PTHREAD_MUTEX_INITIALIZER; 
pthread_mutex_t follow_mutex = PTHREAD_MUTEX_INITIALIZER;


typedef struct thread_parameters{ 
	int sockfd;
	int flag;
	int userid;
}thread_parameters;

typedef struct notification{
	uint32_t id; //Identificador da notificação (sugere-se um identificador único)
 	uint32_t timestamp; //Timestamp da notificação
 	uint16_t length; //Tamanho da mensagem
 	uint16_t pending; //Quantidade de leitores pendentes
 	const char* _string; //Mensagem
}notification;

//ctrl c handle
void signalHandler(int signal) {
   close(sockfd);   
//   save_profiles(profile_list);
   printf("\nServer ended successfully\n");
   exit(0);
}


// IMPLEMENTAR ESSES DOIS
void sendhandler();

void followhandler();

void *clientmessagehandler(void *arg){
	thread_parameters *par = (thread_parameters*) arg;
	int userid = par->userid;
	int sockfd = par->sockfd;
	char newfollow[21];
	packet msg;

	recvpacket(sockfd, &msg, serv_addr);

	while(par->flag){
		switch(msg.type){
			case QUIT:
				close(sockfd);
				par->flag = 0;
				//TEM QUE COLOCAR BARREIRA !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
				break;
			
			case SEND:
				pthread_mutex_lock(&send_mutex);
				sendhandler();
				pthread_mutex_unlock(&send_mutex);
				break;
			
			case FOLLOW:
				pthread_mutex_lock(&follow_mutex);
				strcpy(newfollow, msg._payload);
				followhandler();
				pthread_mutex_unlock(&follow_mutex);
				break;
			
			default:
				printf("Unknown message type received.");
				exit(1);
				break;
		}
		free(msg._payload);
			
	}
	return;
}

// IMPLEMENTAR
void *notificationhandler(void *arg){
	return;
}

int main(int argc, char *argv[])
{
	int one = 1;
	int sockfd, i; 
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;
	packet msg;
	thread_parameters threadparams[CLIENTLIMIT];
	pthread_t client_pthread[2*CLIENTLIMIT];
		
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		printf("ERROR opening socket");
	}
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	bzero(&(serv_addr.sin_zero), 8);    

	if ((setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) == -1)){
		printf("Failed setsockopt.");
		exit(1);
	}

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) < 0) 
		printf("ERROR on binding");

	clilen = sizeof(struct sockaddr_in);

	printf("Server initialized");
	
	while (TRUE) {
		signal(SIGINT, signalHandler);
		/* receive from socket */
		recvpacket(sockfd, &msg, serv_addr);
		if (msg.type != LOGUSER){
			printf("User not logged in.");
			exit(1);
		}

		if((pthread_create(&client_pthread[i], NULL, clientmessagehandler, &threadparams[i]) != 0 )){
			printf("Client message handler thread did not open succesfully.\n");
		}
		
		if((pthread_create(&client_pthread[i+1], NULL, notificationhandler, &threadparams[i]) != 0 )){
			printf("Notification handler thread did not open succesfully.\n");
		}
		i+=2;
	}
	
	close(sockfd);
	return 0;
}