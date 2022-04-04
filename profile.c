#ifndef PROFILE_H

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#define PROFILE_H
#define MAX_CLIENTS 500
#define MAX_FOLLOW 500

/////////////////notification.h + notification.c///////////

#define MAX_NOTIFS 500

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct notification{
	uint32_t id; //Identificador da notificação (sugere-se um identificador único)
 	uint32_t timestamp; //Timestamp da notificação
 	uint16_t length; //Tamanho da mensagem
 	uint16_t pending; //Quantidade de leitores pendentes
 	char* _string; //Mensagem
	char* sender;  //Usuário que enviou a mensagem    
}	notification;

typedef struct ids_notification{

int userid_notif;  // Who sent the notification
int id_notif;  //The id of the notification

}ids_notification;

void printNotif(notification notif){ //           sender não definido //
    printf("[%.0i:%2.0i] %s - %s\n", notif.timestamp/100, notif.timestamp%100, notif.sender, notif._string);
}

////////////////////////profile.h///////////////////

typedef struct profile{
 char* user_name;                  // Profile identifier (@(...))
 int   online_sessions;            // Number of sessions open with this specific user
 
 int follower_count;             //Number of followers of this user
 struct profile* followed_users[MAX_FOLLOW];  // List of people that follow his profile

 int sent_notif_count; //Number of sent notifs
 notification* list_send_notif[MAX_NOTIFS]; // List of send notifs
 
 int pending_notif_count; 
 ids_notification list_pending_notif[MAX_NOTIFS]; // List of notification identifiers
 
 } profile;

/////pode abrir cabeçalhos do profile.c//////
int profile_handler(profile *list_of_profiles, char *username, int newsockfd, int sqncnt);
void profiles_initializer(profile *list_of_profiles); //Loads all the starting profiles
int insert_profile(profile *list_of_profiles, char* username); //Inserts a new profile
int get_profile_id(profile *list_of_profiles, char *username); //Gets a profile bid by name

void print_profile_pointers(profile** profile_pointers);
void print_pnd_notifs(profile p);
void print_profiles(profile* profile_list);


#endif