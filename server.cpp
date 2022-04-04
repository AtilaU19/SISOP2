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




//ctrl c handle
void signalHandler(int signal) {
   close(sockfd);   
//   save_profiles(profile_list);
   printf("\nServer ended successfully\n");
   exit(0);
}

int main(int argc, char *argv[])
{
	int one = 1;
	int sockfd; 
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;
	packet msg;
		
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) 
		printf("ERROR opening socket");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	bzero(&(serv_addr.sin_zero), 8);    

	if ((setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) == -1)){
		printf("Failed setsockopt.");
		exit(1);
	}
	printf("chegou 1");
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) < 0) 
		printf("ERROR on binding");
	printf("chegou 2");
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

		exit(0);
		/* send to socket */
//		n = sendto(sockfd, "Got your message\n", 17, 0,(struct sockaddr *) &cli_addr, sizeof(struct sockaddr));
//		if (n  < 0) 
//			printf("ERROR on sendto");
	}
	
	close(sockfd);
	return 0;
}