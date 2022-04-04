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
int profile_handler(profile *list_of_profiles, char *username, int sockfd, int sqncnt){//add profile if it doesn't exist, else add to online
	
	int profile_id = get_profile_id(list_of_profiles,username);

	if(profile_id == -1){ //CASO NÃO EXISTA 

		//INSERE
		profile_id = insert_profile(list_of_profiles, username);			//insere novo profile na lista

		if(profile_id == -1){											//veririca se vaor max. não foi excedido
			printf("MAX NUMBER OF PROFILES REACHED\n");
			exit(1);
		}
	} 
	else{
		if(list_of_profiles[profile_id].online > 1){ 					//se o usuário já existe, verifica-se se não excedeu o número máximo de 2 sessões online por usuário
			
			printf("Um usuario tentou exceder o numero de acessos.\n");
			send_packet(sockfd,CMD_QUIT,sqncnt,4,0,"quit");			//caso excedido, sistema da quit e fecha socket
			close(sockfd);
			return -1;
		}
		else{
			list_of_profiles[profile_id].online +=1;				//aumenta em 1 a quantia de usuários online
		}
	}

	return profile_id;
}

void profiles_initializer(profile *list_of_profiles){		//inicializa todas profiles da lista com nome vazio e 0 em online			
		for(int i =0; i<MAX_CLIENTS; i++){
			list_of_profiles[i].name= "";
			list_of_profiles[i].online= 0;
	}
} 

void print_profiles(profile* list_of_profiles){				//printa todas profiles da lista

	for(int i =0; i<MAX_CLIENTS; i++){
		if(list_of_profiles[i].name != ""){
			printf("Profile: %s Online %d\n", list_of_profiles[i].name, list_of_profiles[i].online);
		}
	}
}

int insert_profile(profile *list_of_profiles, char* username){		//cria uma nova profile
		
		for(int i =0; i<MAX_CLIENTS; i++){								//seta seus valores iniciais
        if(list_of_profiles[i].name == ""){
        	
        	list_of_profiles[i].name = (char*)malloc(strlen(username)+1);
            strcpy(list_of_profiles[i].name,username);
            list_of_profiles[i].online = 1;
            list_of_profiles[i].num_followers = 0;
            list_of_profiles[i].num_snd_notifs = 0;
            list_of_profiles[i].num_pnd_notifs = 0;

            //Initializing pending and send notifications
            for(int j=0;j<MAX_NOTIFS; j++){

            	list_of_profiles[i].pnd_notifs[j].notif_id = -1;
            	list_of_profiles[i].pnd_notifs[j].profile_id = -1;
            	list_of_profiles[i].snd_notifs[j]= NULL;
            }
			
            return i;
        }
    }

    return -1
} 
int get_profile_id(profile *list_of_profiles, char *username){//Gets a profile bid by name
		for(int i =0; i<MAX_CLIENTS; i++){
		if(list_of_profiles[i].name != ""){
			
			if(strcmp(list_of_profiles[i].name,username)== 0){
				return i;
			}
		}
	}
	return -1;
} 

void print_profile_pointers(profile** profile_pointers){ //
		profile p;
	for(int i =0; i<MAX_FOLLOW; i++){
		p = (*profile_pointers)[i];
		if(p.name != ""){
			printf("Profile: %s Online %d\n", p.name, p.online);
		}
	}
}

void print_pnd_notifs(profile p){ //printa notificações pendentes do usuário
		profile p;
	for(int i =0; i<MAX_FOLLOW; i++){
		p = (*profile_pointers)[i];
		if(p.name != ""){
			printf("Profile: %s Online %d\n", p.name, p.online);
		}
	}
}

#endif