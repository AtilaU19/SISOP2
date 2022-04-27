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
#include "packet.c"

int port;
int seqncnt = 0;
int sockfd;
struct hostent *server;
struct sockaddr_in serv_addr, cli_addr;

void signal_handler(int signal)
{
	sendpacket(sockfd, QUIT, seqncnt++, 0, getcurrenttime(), "", serv_addr);
	close(sockfd);
	printf("Sessão encerrada\n");
	exit(1);
}

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

// fecha a session quando der ctrl c no terminal
void closeSession(int sockfd)
{
	sendpacket(sockfd, QUIT, ++seqncnt, 0, 0, "", serv_addr);
	close(sockfd);
	system("clear");
	printf("Session closed\n");
	exit(1);
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
		printf("Estou no send message e o socket é %i\n", sockfd);
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
				sendpacket(sockfd, SEND, ++seqncnt, strlen(buffer) - 5, getcurrenttime(), buffer + 5 * sizeof(char), serv_addr);
			}
			break;
		case FOLLOW:
			sendpacket(sockfd, FOLLOW, ++seqncnt, strlen(buffer) - 7, getcurrenttime(), buffer + 7 * sizeof(char), serv_addr);
			break;
		default:
			printf("Action unknown. Should be:\nSEND <message>\nFOLLOW <@user>\nQUIT\n");
		}
	}
}

int change_port(char *newport, int oldsockfd)
{
	int newsockfd, newportint = atoi(newport);
	close(oldsockfd);
	printf("Changing port from %i to %s\n", port, newport);
	if ((newsockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		printf("ERROR opening socket");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(newportint);
	serv_addr.sin_addr = *((struct in_addr *)server->h_addr);
	bzero(&(serv_addr.sin_zero), 8);

	return newsockfd;
}

void *receivemessage(void *arg)
{
	// int sockfd = *(int *) arg;
	char *acknowledge = "ack";
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
			close(sockfd);
			exit(1);
			break;
		case CHANGEPORT:
			printf("Server accepted login attempt, changing port\n");
			sockfd = change_port(msg._payload, sockfd);
			sendpacket(sockfd, ACK, seqncnt++, strlen(acknowledge), getcurrenttime(), acknowledge, serv_addr);
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
		printf("Handle must start with @\nExample: @TestUser");
	}
}

int main(int argc, char *argv[])
{
	pthread_t thr_client_input, thr_client_display;

	int n;
	unsigned int length;
	char handle[20];

	if (argc < 4)
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

	port = atoi(argv[3]);

	strcpy(handle, argv[1]);
	validateuserhandle(handle);
	printf("User handle: %s , Port: %d\nUse SEND to send a message to all followers.\nUse FOLLOW <@handle> to follow another user.\n\n", handle, port);

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		printf("ERROR opening socket");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	serv_addr.sin_addr = *((struct in_addr *)server->h_addr);
	bzero(&(serv_addr.sin_zero), 8);

	sendpacket(sockfd, LOGUSER, ++seqncnt, strlen(handle) + 1, getcurrenttime(), handle, serv_addr);

	// Começa uma thread para receber mensagens e outra para enviar
	pthread_create(&thr_client_display, NULL, receivemessage, &sockfd);
	pthread_create(&thr_client_input, NULL, sendmessage, &sockfd);

	pthread_join(thr_client_display, NULL);
	pthread_join(thr_client_input, NULL);
	close(sockfd);
	return 0;
}