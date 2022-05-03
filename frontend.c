#include "frontend.h"
#include "commons.h"
#include "packet.c"

struct frontend_params{
    char host[50];
    int primary_port;
	char handle[50];
}frontend_params;

int frontend_primary_socket;
int client_frontend_socket;

int frontend_port = -1;
int primary_server_port = - 1;
int frontend_primary_socket;
int client_frontend_socket;
int clilen, seqncnt, logged = 0;

char* connection_estabilished = "connection estabilished with frontend\n";
char *acknowledge = "ack";

struct hostent *server;
struct sockaddr_in serv_addr, cli_addr_front, primary_addr;

pthread_t thr_client_to_primary_server;
pthread_t thr_primary_server_to_client;



void *receive_new_primary(void *arg){
	return(0);
}


int get_frontend_port(){

	//frontend might not have found the port yet, waiting for it to find
	while(frontend_port == -1);
	sleep(2);
	return frontend_port;
    
}
 
int change_port(char *newport, int oldsocket)
{
	int newsocket, newportint = atoi(newport);
	close(oldsocket);
	//printf("-----FRONTEND-----\nChanging port from %i to %s\n------------------\n", primary_server_port, newport);
	if ((newsocket = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		printf("ERROR opening socket");

	primary_addr.sin_family = AF_INET;
	primary_addr.sin_port = htons(newportint);
	primary_addr.sin_addr = *((struct in_addr *)server->h_addr);
	bzero(&(primary_addr.sin_zero), 8);

	return newsocket;

}
 
//Envia as mensagens so client para o server
void *client_to_primary_server(void *arg){

	packetReturn pr;

	sendpacket(frontend_primary_socket, ACK, seqncnt++, strlen(acknowledge)+1, getcurrenttime(), acknowledge, primary_addr);
	//Receive message
    packet message;
    while(1){	
        pr = recvpacket(client_frontend_socket, &message, cli_addr_front);
		if (pr.success){
			cli_addr_front = pr.addr;
			//printf("-----FRONTEND-----\nReceived \"%s\" from client, sending to server.\n------------------\n", message._payload);
			sendpacket(frontend_primary_socket,message.type, message.seqn, message.length, message.timestamp,message._payload, primary_addr);
			free(message._payload);     
			} 
        }
    }

//Envia as mensagens do server para o client
void *primary_server_to_client(void *arg){

	packetReturn pr;

	//Recebe mensagem
    packet message;
    while(1){
    	pr = recvpacket(frontend_primary_socket, &message, primary_addr);
		primary_addr = pr.addr;
		if(pr.success){
			//printf("-----FRONTEND-----\nReceived \"%s\" from server, sending to client.\n------------------\n", message._payload);
			sendpacket(client_frontend_socket,message.type, message.seqn, message.length, message.timestamp,message._payload, cli_addr_front);
			free(message._payload);
		}
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
	int aux_port = (rand() % (5000 - 4050 + 1)) + 4050;
   
    //cria e abre o socket
    if((client_frontend_socket = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
		printf("ERROR opening socket\n");
		return(0);
	} 

    bzero((char *) &serv_addr, sizeof(serv_addr)); 
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;

    //Encontrar um port específico pra esse frontend
    while(!flag){
    	aux_port = (rand() % (5000 - 4050 + 1)) + 4050;
	   	//printf("tentando abrir o port\n");
	    serv_addr.sin_port = htons(aux_port);
	    
	    //BIND TO HOST
	    if(setsockopt(client_frontend_socket, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) == -1){
			printf("ERROR on setsockopt\n");
			exit(0);
		} 
	  	 	
	  	//Se não conseguir fazer bind tenta para outro port
	    if( bind(client_frontend_socket, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) >= 0){
	    	//Se funcionar, liga a flag e sai do while
	  		flag = 1;
	    }
	}
	
	frontend_port = aux_port;
	printf("Frontend port: %i\n",frontend_port);

	//bloqueia esperando o login attempt do usuario
	//
	//sendpacket(client_frontend_socket, LOGUSER, ++seqncnt, strlen(par->handle) + 1, getcurrenttime(), par->handle, primary_addr);
	cli_addr_front = recvpacket(client_frontend_socket, &msg, cli_addr_front).addr;
	//sendpacket(client_frontend_socket, ACK, seqncnt++, strlen(connection_estabilished), getcurrenttime(), connection_estabilished, cli_addr_front);
	//TALVEZ ELE ESTEJA FAZENDO ALGO ERRADO NO RECVPACKET
	//printf("recebeu o packet com isso aqui: %s\n", msg._payload);

    ////////////////////////////////////////////////////////////////////////////////////
    //GET FRONTEND_PRIMARY SOCKET
	if((frontend_primary_socket = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
		printf("ERROR opening socket\n");
	}  
    //CONNECT TO FRONTEND_PRIMARY SOCKET
	primary_addr.sin_family = AF_INET;     
	primary_addr.sin_port = htons(primary_server_port);    
	primary_addr.sin_addr = *((struct in_addr *)server->h_addr);
	bzero(&(primary_addr.sin_zero), 8);     

	//////////////////////////////////////////////

	sendpacket(frontend_primary_socket, msg.type, seqncnt++, strlen(msg._payload) + 1, getcurrenttime(), msg._payload, primary_addr);
	while(!logged){
		serv_addr = recvpacket(frontend_primary_socket, &msg, serv_addr).addr;
	if (msg.type == CHANGEPORT){
			printf("Server accepted login attempt, changing port\n");
			frontend_primary_socket = change_port(msg._payload, frontend_primary_socket);
			logged = 1;
		}
	}
	
    pthread_create(&thr_client_to_primary_server, NULL, client_to_primary_server,NULL);
    pthread_create(&thr_primary_server_to_client, NULL, primary_server_to_client,NULL);
    //pthread_create(&thr_receive_new_primary, NULL, receive_new_primary,NULL);
    pthread_join(thr_client_to_primary_server,NULL);
    pthread_join(thr_primary_server_to_client, NULL);
    //pthread_join(thr_receive_new_primary, NULL);

}