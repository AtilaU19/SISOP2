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
#include <arpa/inet.h>
#include "commons.h"
#include "packet.c"
#include "profile_notif.c"
#include "filehandlers.c"

#define LOGINPORT 4000
#define COMMPORT 10000
#define SIGINT 2
typedef struct thread_parameters
{
	int sockfd;
	int flag;
	int userid;
	struct sockaddr_in cli_addr;

} thread_parameters;

int n, seqncount, i = 1;
struct sockaddr_in serv_addr;
packet msg;

thread_parameters threadparams[CLIENTLIMIT];
pthread_t client_pthread[2 * CLIENTLIMIT];

pthread_mutex_t send_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t follow_mutex = PTHREAD_MUTEX_INITIALIZER;
// barreira do consumidor//
pthread_barrier_t barriers[CLIENTLIMIT];

// lista de profiles//
profile list_of_profiles[CLIENTLIMIT];

// ctrl c handle
void signalHandler(int signal)
{
	//   close(sockfd);
	save_profiles(list_of_profiles);
	printf("\n***** SERVER ENDED *****\n");
	exit(0);
}

///////////////verifica se o follow é válido//////////////////
int is_follow_valid(int followid, int userid, char *user_name, int sockfd)
{
	char payload[100];
	// return false if user already follows them
	for (int i = 0; i < list_of_profiles[followid].follower_count; i++)
	{
		if (strcmp(list_of_profiles[followid].followed_users[i]->user_name, list_of_profiles[userid].user_name) == 0)
		{
			return 0;
		}
	}

	// return false if user does not exist in list of users
	if (followid == -1)
	{
		return 0;
	}

	// return false if user tries to follow themselves
	if (strcmp(user_name, list_of_profiles[userid].user_name) == 0)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

void followhandler(char *user_name, int userid, int sockfd)
{
	int followid;
	int followercount;
	//
	char *loggedusername = list_of_profiles[userid].user_name;

	// pega o usuario que está sendo seguido
	followid = get_profile_id(list_of_profiles, user_name);

	// check if followed exists and is not already followed/is not the user
	if (!is_follow_valid(followid, userid, user_name, sockfd))
		return;

	// check if follower has not exceeded follower limit
	followercount = list_of_profiles[followid].follower_count;
	if (followercount >= FOLLOWLIMIT)
	{
		printf("User has reached max followers and could not follow %s", user_name);
		exit(1);
	}
	printf("User %s just followed %s \n", loggedusername, user_name);
	// ADD FOLLOWER
	list_of_profiles[followid].follower_count++;
	list_of_profiles[followid].followed_users[followercount] = &list_of_profiles[userid];
	// isso aqui ta ao contrario

	return;
}

void sendhandler(notification *notif, packet msg, int userid, int sockfd)
{

	profile *p;
	int pending_notif_count;
	int num_followers = list_of_profiles[userid].follower_count;
	int id_notif;

	// Update notif id
	id_notif = list_of_profiles[userid].sent_notif_count;
	list_of_profiles[userid].sent_notif_count++;
	// Create notification
	notif = malloc(sizeof(notification));
	notif->id = id_notif;
	notif->sender = list_of_profiles[userid].user_name;
	notif->timestamp = msg.timestamp;
	notif->_string = (char *)malloc(strlen(msg._payload) * sizeof(char) + sizeof(char));
	memcpy(notif->_string, msg._payload, strlen(msg._payload) * sizeof(char) + sizeof(char));
	notif->length = msg.length;
	notif->pending = list_of_profiles[userid].follower_count;

	// printf("Payload da notificação = %s\n", notif->_string);

	// Putting the notification on the current profile as send
	list_of_profiles[userid].list_send_not[id_notif] = notif;

	// Putting the notification on followers pending list
	for (int i = 0; i < num_followers; i++)
	{

		p = list_of_profiles[userid].followed_users[i];

		pending_notif_count = p->pending_notif_count;
		p->pending_notif_count++;

		if (p->pending_notif_count == NOTIFLIMIT)
		{
			p->pending_notif_count = 0;
		}

		p->list_pending_not[pending_notif_count].id_notif = id_notif;
		p->list_pending_not[pending_notif_count].userid_notif = userid;
	}
}

void *clientmessagehandler(void *arg)
{
	thread_parameters *par = (thread_parameters *)arg;
	int userid = par->userid;
	char newfollow[21];
	packet msg;
	notification *notif;

	printf("opened clientmessagehandler for user %s\n", list_of_profiles[userid].user_name);

	while (par->flag)
	{
		par->cli_addr = recvpacket(par->sockfd, &msg, par->cli_addr);
		// printf("estourou depois do recvpacket\n");

		switch (msg.type)
		{
		case QUIT:
			list_of_profiles[userid].online_sessions -= 1;											 // remove usuário da lista de sessões abertas
			pthread_barrier_init(&barriers[userid], NULL, list_of_profiles[userid].online_sessions); // remove sessão das barriers, evita que um client espere por outro do mesmo usuário que já fechou
			close(par->sockfd);
			par->flag = 0;
			break;

		case SEND:
			pthread_mutex_lock(&send_mutex);
			sendhandler(notif, msg, userid, par->sockfd);
			pthread_mutex_unlock(&send_mutex);
			break;
			// SEND GERA CORRUPTED TOP SIZE não é problema nos mutex já testei
		case FOLLOW:
			pthread_mutex_lock(&follow_mutex);
			strcpy(newfollow, msg._payload);
			followhandler(newfollow, userid, par->sockfd);
			pthread_mutex_unlock(&follow_mutex);
			break;
		// FOLLOW GERA DOUBLE FREE DETECTED IN TCACHE2
		default:
			printf("Unknown message type received.\n");
			printf("Message data: %i, %i, %i, %i, %s\n", msg.type, msg.seqn, msg.length, msg.timestamp, msg._payload);
			exit(1);
			break;
		}
	}
	free(msg._payload);
}

// handles client consumes (sending the notifications from their followers)
void *notificationhandler(void *arg)
{
	thread_parameters *par = (thread_parameters *)arg;
	int sockfd = par->sockfd;
	int userid = par->userid;
	ids_notification notifid;
	profile *user = &list_of_profiles[userid];
	notification *notif;
	char payload[BUFFER_SIZE];
	char buffer[INET_ADDRSTRLEN];

	printf("opened notificationhandler for user %s\n", list_of_profiles[userid].user_name);
	// using the same flag as other thread to maintain both synced up
	while (par->flag)
	{
		// for each pending notification send message to user
		for (int i = 0; i < user->pending_notif_count; i++)
		{
			notifid = user->list_pending_not[i];
			// if notification exists then sends
			if (notifid.userid_notif != -1)
			{
				// gets notification
				notif = list_of_profiles[notifid.userid_notif].list_send_not[notifid.id_notif];
				// payload = malloc(notif->length+strlen(notif->sender) + 12*sizeof(char));
				// printf("[+] DEBUG server > sizeof payload for notification: %lu\n", sizeof(payload));
				sprintf(payload, "[%.0i:%02d] %s - %s", notif->timestamp / 100, notif->timestamp % 100, notif->sender, notif->_string);
				// sprintf(payload,"%s - %s", notif->sender, notif->_string);
				// talvez o sprintf seja necessário pra colocar o valor na payload mas vamo ver
				printf("Payload = %s\n", payload);
				// sends notification

				inet_ntop(AF_INET, &par->cli_addr.sin_addr, buffer, sizeof(buffer));
				printf("[+] DEBUG server > sockfd = %i, send address = %s\n", sockfd, buffer);
				// TA PEGANDO O ENDEREÇO ERRADO QUANDO N TA FEITO O FOLLOW

				sendpacket(sockfd, NOTIFICATION, ++seqncount, strlen(payload) + 1, getcurrenttime(), payload, par->cli_addr);
				// if(payload){
				//	free(payload);
				// }

				// DOIS ERROS ACONTECENDO:
				// A mensagem está sendo enviada para o seguido, não para o seguidor. --RESOLVIDO
				// O erro de floating point é decorrente das barriers ou do mutex, não sei qual ainda
				// Além disso o conteúdo da mensagem tá só com o horário, sem o sender ou o conteúdo de fato --RESOLVIDO

				// barrerira para usuário com mais de um client
				// pthread_barrier_wait (&barriers[userid]);

				// locks mutex to change list of notifications without having other thread take from it at the same time
				// pthread_mutex_lock(&send_mutex);

				if (par->flag)
				{
					// see if notification has not been deleted by another client of same user
					if (user->list_pending_not[i].userid_notif != -1)
					{

						notif->pending--;
						if (notif->pending == 0)
						{
							// user no longer has notifications, notification is deleted
							notif = NULL;
						}

						// deletes notification from users list of pending notifications
						user->list_pending_not[i].userid_notif = -1;
						user->list_pending_not[i].id_notif = -1;
					}
				}

				// unlocks thread after removing notification from list, avoids conflicts between threads
				// pthread_mutex_unlock(&send_mutex);
			}
		}
	}
}

void init_barriers()
{ // inicializa barreiras

	for (int i = 0; i < CLIENTLIMIT; i++)
	{
		pthread_barrier_init(&barriers[i], NULL, 0);
	}
}

void login_handler(int userid, int sockfd, struct sockaddr_in cli_addr)
{
	char buffer[BUFFER_SIZE];
	int newsocket, juan = 1;
	struct sockaddr_in newcli_addr;
	char debug[BUFFER_SIZE];
	if (userid != -1)
	{

		sprintf(buffer, "%i", COMMPORT + i);
		printf("New port for user %s: %s\n", list_of_profiles[userid].user_name, buffer);
		sendpacket(sockfd, CHANGEPORT, seqncount++, strlen(buffer) + 1, getcurrenttime(), buffer, cli_addr);

		if ((newsocket = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		{
			printf("ERROR opening socket");
		}
		bzero((char *)&newcli_addr, sizeof(newcli_addr));
		newcli_addr.sin_family = AF_INET;
		newcli_addr.sin_addr.s_addr = INADDR_ANY;
		newcli_addr.sin_port = htons(COMMPORT + i);

		if ((setsockopt(newsocket, SOL_SOCKET, SO_REUSEADDR, &juan, sizeof(juan)) == -1))
		{
			printf("Failed setsockopt.");
			exit(1);
		}

		if (bind(newsocket, (struct sockaddr *)&newcli_addr, sizeof(struct sockaddr)) < 0)
			printf("ERROR on binding");

		// printf("DEBUG: Old socket: %i New socket: %i\n", sockfd, newsocket);
		/*
		inet_ntop( AF_INET, &cli_addr.sin_addr, debug, sizeof( debug ));
		printf("[+] DEBUG server > sockfd = %i, CLIENT address = %s\n", sockfd, debug);
		*/
		newcli_addr = recvpacket(newsocket, &msg, newcli_addr);
		/*
		inet_ntop( AF_INET, &newcli_addr.sin_addr, debug, sizeof( debug ));
		printf("[+] DEBUG server > sockfd = %i, NEW address = %s\n", sockfd, debug);
		*/

		threadparams[i].userid = userid;
		threadparams[i].sockfd = newsocket;
		threadparams[i].flag = 1;
		threadparams[i].cli_addr = newcli_addr;

		if ((pthread_create(&client_pthread[i], NULL, clientmessagehandler, &threadparams[i]) != 0))
		{
			printf("Client message handler thread did not open succesfully.\n");
		}

		if ((pthread_create(&client_pthread[i + 1], NULL, notificationhandler, &threadparams[i]) != 0))
		{
			printf("Notification handler thread did not open succesfully.\n");
		}

		i += 2;
	}

	// exit(1);
}

int main(int argc, char *argv[])
{
	int one = 1;
	int userid, sockfd;
	socklen_t clilen;
	struct sockaddr_in cli_addr;

	// incialização das structs
	init_profiles(list_of_profiles);
	read_profiles(list_of_profiles);
	init_barriers();

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		printf("ERROR opening socket");
	}
	bzero((char *)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(LOGINPORT);

	if ((setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) == -1))
	{
		printf("Failed setsockopt.");
		exit(1);
	}

	if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) < 0)
		printf("ERROR on binding");

	clilen = sizeof(struct sockaddr_in);

	printf("***** SERVER INITIALIZED *****\n");

	while (1)
	{
		signal(SIGINT, signalHandler);
		/* receive from socket */
		cli_addr = recvpacket(sockfd, &msg, cli_addr);

		if (msg.type == LOGUSER)
		{
			userid = profile_handler(list_of_profiles, msg._payload, sockfd, ++seqncount);
			free(msg._payload);
			// printf("[+] DEBUG server > ID DO USER %i \n", userid);

			login_handler(userid, sockfd, cli_addr);
		}
		else
		{
			printf("[+] Received this instead of login packet: %i, %i, %i, %i, %s\n", msg.type, msg.seqn, msg.length, msg.timestamp, msg._payload);
			printf("User not logged in.\n");
		}
	}
	close(sockfd);
	return 0;
}