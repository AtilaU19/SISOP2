#ifndef PROFILE_H
#define PROFILE_H
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "commons.h"

typedef struct notification
{
	uint32_t id;		// Identificador da notificação (sugere-se um identificador único)
	uint32_t timestamp; // Timestamp da notificação
	uint16_t length;	// Tamanho da mensagem
	uint16_t pending;	// Quantidade de leitores pendentes
	char *_string;		// Mensagem
	char *sender;		// Usuário que enviou a mensagem
} notification;

typedef struct ids_notification
{

	int userid_notif; // Who sent the notification
	int id_notif;	  // The id of the notification

} ids_notification;

void printNotif(notification notif)
{ //           sender não definido //
	printf("[%.0i:%2.0i] %s - %s\n", notif.timestamp / 100, notif.timestamp % 100, notif.sender, notif._string);
}

typedef struct profile
{
	char *user_name;	 // Profile identifier (@(...))
	int online_sessions; // Number of sessions open with this specific user

	int follower_count;							 // Number of followers of this user
	struct profile *followed_users[FOLLOWLIMIT]; // List of people that follow his profile

	int sent_notif_count;					 // Number of sent notifs
	notification *list_send_not[NOTIFLIMIT]; // List of send notifs

	int pending_notif_count;
	ids_notification list_pending_not[NOTIFLIMIT]; // List of notification identifiers

} profile;

int insert_profile(profile *list_of_profiles, char *username)
{ // cria uma nova profile

	for (int i = 0; i < CLIENTLIMIT; i++)
	{ // seta seus valores iniciais
		if (list_of_profiles[i].user_name == "")
		{

			list_of_profiles[i].user_name = (char *)malloc(strlen(username) + 1);
			strcpy(list_of_profiles[i].user_name, username);
			list_of_profiles[i].online_sessions = 1;
			list_of_profiles[i].follower_count = 0;
			list_of_profiles[i].sent_notif_count = 0;
			list_of_profiles[i].pending_notif_count = 0;

			// Initializing pending and send notifications
			for (int j = 0; j < NOTIFLIMIT; j++)
			{

				list_of_profiles[i].list_pending_not[j].id_notif = -1;
				list_of_profiles[i].list_pending_not[j].userid_notif = -1;
				list_of_profiles[i].list_send_not[j] = NULL;
			}

			return i;
		}
	}

	return -1;
}

int get_profile_id(profile *list_of_profiles, char *username)
{ // Gets a profile bid by name
	for (int i = 0; i < CLIENTLIMIT; i++)
	{
		if (list_of_profiles[i].user_name != "")
		{

			if (strcmp(list_of_profiles[i].user_name, username) == 0)
			{
				return i;
			}
		}
	}
	return -1;
}

int profile_handler(profile *list_of_profiles, char *username, int sockfd, int seqncnt)
{ // add profile if it doesn't exist, else add to online

	int userid = get_profile_id(list_of_profiles, username);

	if (userid == -1)
	{ // CASO NÃO EXISTA

		// INSERE
		userid = insert_profile(list_of_profiles, username); // insere novo profile na lista

		if (userid == -1)
		{ // veririca se vaor max. não foi excedido
			printf("MAX NUMBER OF PROFILES REACHED\n");
			exit(1);
		}
	}
	else
	{
		if (list_of_profiles[userid].online_sessions > 1)
		{ // se o usuário já existe, verifica-se se não excedeu o número máximo de 2 sessões online por usuário

			printf("Um usuario tentou exceder o numero de acessos.\n");
			// caso excedido, sistema da quit e fecha socket
			close(sockfd);
			return -1;
		}
		else
		{
			list_of_profiles[userid].online_sessions += 1; // aumenta em 1 a quantia de usuários online
		}
	}

	return userid;
}

void init_profiles(profile *list_of_profiles)
{ // inicializa todas profiles da lista com nome vazio e 0 em online
	for (int i = 0; i < CLIENTLIMIT; i++)
	{
		list_of_profiles[i].user_name = "";
		list_of_profiles[i].online_sessions = 0;
	}
}

#endif