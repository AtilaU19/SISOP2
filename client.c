#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include "commons.h"
#include "frontend.c"


int port_front;
int seqncnt = 0;
int sockfd;
char disconnecting[BUFFER_SIZE];
struct hostent *server;
struct sockaddr_in frontend_addr, cli_addr;
pthread_mutex_t frontend_mutex = PTHREAD_MUTEX_INITIALIZER;

int getaction(char *buffer)
{
	if (!strncmp(buffer, "SEND ", 5))
		return SEND;

	if (!strncmp(buffer, "FOLLOW ", 7))
		return FOLLOW;
	// se não for follow ou send retorna erro (-1)
	else
		return -1;
}

// fecha a session quando excedeu o número de sessões limite
void closeSession(int sockfd)
{
	sendpacket(sockfd, QUIT, seqncnt++, strlen(disconnecting), getcurrenttime(), disconnecting, frontend_addr);
	close(sockfd);
	system("clear");
	printf("Session closed\n");
	exit(1);
}

void signal_handler(int signal)
{
	closeSession(sockfd);
}

// function to send messages from client
// should be called in a separate thread
// as to not conflict with receiving messages
void *sendmessage(void *arg)
{
	int n, flag, action;
	int sockfd = *(int *)arg;
	char buffer[BUFFER_SIZE];

	while (TRUE)
	{
		//printf("Estou no send message e o socket é %i\n", sockfd);
		// clears buffer
		bzero(buffer, BUFFER_SIZE);
		if (!fgets(buffer, BUFFER_SIZE, stdin))
			action = QUIT;
		else
			action = getaction(buffer);
		switch (action)
		{
		case QUIT:
			// isso aqui vai ser o handle de sair da sessão
			closeSession(sockfd);
			break;
		case SEND:
			if (strlen(buffer) > 134)
			{
				printf("Message exceeds character limit (128)\n");
			}
			else
			{
				sendpacket(sockfd, SEND, ++seqncnt, strlen(buffer) - 5, getcurrenttime(), buffer + 5 * sizeof(char), frontend_addr);
			}
			break;
		case FOLLOW:
			sendpacket(sockfd, FOLLOW, ++seqncnt, strlen(buffer) - 7, getcurrenttime(), buffer + 7 * sizeof(char), frontend_addr);
			break;
		default:
			printf("Action unknown. Should be:\nSEND <message>\nFOLLOW <@user>\nQUIT\n");
		}
	}
}


void *receivemessage(void *arg)
{
	// int sockfd = *(int *) arg;
	packet msg;

	signal(SIGINT, signal_handler);

	while (TRUE)
	{
		// printf("Estou no receive package e o socket é %i\n", sockfd);
		cli_addr = recvpacket(sockfd, &msg, cli_addr);
		// printf("[+] DEBUG client > received %s\n", msg._payload);
		switch (msg.type)
		{
		case FOLLOW:
			printf("%s\n", msg._payload);
			break;
		case NOTIFICATION:
			printf("%s\n", msg._payload);
			break;
		case QUIT:
			printf("Simultaneous session limit exceeded.");
			closeSession(sockfd);
			break;
		case CHANGEPORT:
			break;
		default:
			printf("Unknown message error");
			break;
		}
		free(msg._payload);
	}
}

void validateuserhandle(char *handle)
{
	// Testa o comprimento do username
	if (!(strlen(handle) <= 20 && strlen(handle) >= 4))
	{
		printf("Username must have between 4 and 20 characters.");
		exit(0);
	}
	// Testa se o primeiro caractere é @
	if ((handle[0] != '@'))
	{
		printf("Handle must start with @\nExample: @TestUser\n");
		exit(0);
	}

	sprintf(disconnecting,"User %s disconnected.", handle);
}

int main(int argc, char *argv[])
{
	pthread_t thr_client_input, thr_client_display, thr_frontend_startup;

	int n, serv_primary_port;
	unsigned int length;
	char handle[50];

	if (argc != 4)
	{
		fprintf(stderr, "usage ./app_cliente <@profile> <server_address> <port> \n");
		exit(0);
	}

	server = gethostbyname(argv[2]);
	if (server == NULL)
	{
		fprintf(stderr, "ERROR, no such host\n");
		exit(0);
	}

	serv_primary_port = atoi(argv[3]);

	strcpy(handle, argv[1]);
	validateuserhandle(handle);

	printf("User handle: %s , Port: %d\nUse SEND to send a message to all followers.\nUse FOLLOW <@handle> to follow another user.\n\n", handle, serv_primary_port);

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		printf("ERROR opening socket");


	//FRONTEND
	struct frontend_params params;
	strcpy(params.handle, handle);
    strcpy(params.host, argv[2]);
    params.primary_port = serv_primary_port;

	pthread_create(&thr_frontend_startup, NULL, frontend_startup, &params);
    port_front = get_frontend_port();
	printf("recebi o port do frontend: %i\n", port_front);

	//depois de conseguir o socket do frontend conecta com ele

	frontend_addr.sin_family = AF_INET;
	frontend_addr.sin_port = htons(port_front);
	frontend_addr.sin_addr = *((struct in_addr *)server->h_addr);
	bzero(&(frontend_addr.sin_zero), 8);
	printf("vou enviar o login\n");
	sendpacket(sockfd, LOGUSER, ++seqncnt, strlen(handle) + 1, getcurrenttime(), handle, frontend_addr);
	printf("enviei o login\n");
	// Começa uma thread para receber mensagens e outra para enviar
	pthread_create(&thr_client_display, NULL, receivemessage, &sockfd);
	pthread_create(&thr_client_input, NULL, sendmessage, &sockfd);

	pthread_join(thr_client_display, NULL);
	pthread_join(thr_client_input, NULL);
	pthread_join(thr_frontend_startup, NULL);
	close(sockfd);
	return 0;
}