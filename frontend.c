#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <netdb.h> 
#include <signal.h>
#include <time.h>

#include "commons.h"
#include "packet.c"

struct frontend_params{
    char host[50];
    int primary_port;
}frontend_params;

int frontend_primary_socket;
int client_frontend_socket;

int frontend_port = -1;
int primary_server_port = - 1;
int frontend_primary_socket;
int client_frontend_socket;
int sockfd = 0;
int clilen, seqncnt;

struct hostent *server;
struct sockaddr_in serv_addr, cli_addr;

pthread_t thr_client_to_primary_server;
pthread_t thr_primary_server_to_client;



void *receive_new_primary(void *arg){
	return(0);
}


int get_frontend_port(){

	//frontend might not have found the port yet, waiting for it to find
	while(frontend_port == -1);

	return frontend_port;
    
}
 
 
//Envia as mensagens so client para o server
void *client_to_primary_server(void *arg){
	
	//Warn server of this port
	char payload[5];
    sprintf(payload, "%d", get_frontend_port());
	
	//Receive message
    packet message;
    while(1){	
        recvpacket(client_frontend_socket, &message, cli_addr);
        sendpacket(frontend_primary_socket,message.type, message.seqn, message.length, message.timestamp,message._payload, serv_addr);
                /*
				if(message.type == LOGUSER){
                    sleep(0.5);
                    send_packet(frontend_primary_socket, CHANGEPORT, 1, strlen(payload)+1 , getTime(), payload );
                }
				*/
                free(message._payload);      
        }
    }

//Envia as mensagens do server para o client
void *primary_server_to_client(void *arg){

	//Recebe mensagem
    packet message;
    while(1){
    	recvpacket(frontend_primary_socket, &message, serv_addr);
        sendpacket(client_frontend_socket,message.type, message.seqn, message.length, message.timestamp,message._payload, cli_addr);
        free(message._payload);
           
   	}
}

void *frontend_startup(void *arg){

	packet msg;
	pthread_t thr_receive_new_primary;

	struct frontend_params *par = (struct frontend_params *) arg;
	server =  gethostbyname(par->host);
    primary_server_port = par->primary_port;

	
    int one=1,flag=0;
	srand(time(0));
	int aux_port;//Default port
    
    //CREATE SOCKET
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
   
    //OPEN SOCKET  
    if(sockfd < 0){
		printf("ERROR opening socket\n");
		return(0);
	} 

    bzero((char *) &serv_addr, sizeof(serv_addr)); 
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;

    //Encontrar um port específico pra esse frontend
    while(!flag){
    	aux_port = (rand() % (951)) + 4050;
	   
	    serv_addr.sin_port = htons(aux_port);
	    
	    //BIND TO HOST
	    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) == -1){
			printf("ERROR on setsockopt\n");
			exit(0);
		
		} 
	  	 	
	  	//Se não conseguir fazer bind tenta para outro port
	    if( bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) >= 0){
	    	//Se funcionar, liga a flag e sai do while
	  		flag = 1;
	    }
	}
	
	frontend_port = aux_port;


	//GET CLIENT_FRONTEND SOCKET

	cli_addr = recvpacket(sockfd, &msg, cli_addr);

   	clilen = sizeof(cli_addr);
   	//ACCEPT
	sendpacket(sockfd, msg.type, seqncnt++, strlen(msg._payload) + 1, getcurrenttime(), msg._payload, serv_addr);


    ////////////////////////////////////////////////////////////////////////////////////
    //GET FRONTEND_PRIMARY SOCKET
	if((frontend_primary_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		printf("ERROR opening socket\n");
	}  
    //CONNECT TO FRONTEND_PRIMARY SOCKET
	serv_addr.sin_family = AF_INET;     
	serv_addr.sin_port = htons(primary_server_port);    
	serv_addr.sin_addr = *((struct in_addr *)server->h_addr);
	bzero(&(serv_addr.sin_zero), 8);     

	//////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////////
  	//CREATE THREADS
    pthread_create(&thr_client_to_primary_server, NULL, client_to_primary_server,NULL);
    pthread_create(&thr_primary_server_to_client, NULL, primary_server_to_client,NULL);
    pthread_create(&thr_receive_new_primary, NULL, receive_new_primary,NULL);
    pthread_join(thr_client_to_primary_server,NULL);
    pthread_join(thr_primary_server_to_client, NULL);
    pthread_join(thr_receive_new_primary, NULL);

}