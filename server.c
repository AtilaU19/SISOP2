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
#include "packet.c"
#include "profile.c"

#define PORT 4000
#define SIGINT 2
int sockfd, n, seqncount;	
struct sockaddr_in serv_addr, cli_addr;

pthread_mutex_t send_mutex =  PTHREAD_MUTEX_INITIALIZER; 
pthread_mutex_t follow_mutex = PTHREAD_MUTEX_INITIALIZER;

profile list_of_profiles[MAX_CLIENTS];


typedef struct thread_parameters{ 
	int sockfd;
	int flag;
	int userid;

}thread_parameters;
//ctrl c handle
void signalHandler(int signal) {
   close(sockfd);   
//   save_profiles(profile_list);
   printf("\nServer ended successfully\n");
   exit(0);
}

int is_follow_valid(int followid,int userid, char *user_name, int sockfd){
   char payload[100];
//return false if user already follows them
   for(int i=0;i<list_of_profiles[followid].follower_count;i++){
      if(strcmp(list_of_profiles[followid].followed_users[i]->user_name,list_of_profiles[userid].user_name)==0){
         return 0;
    	}

   	}
//return false if user does not exist in list of users
   if(followid == -1){ 
      return 0;
   	}
//return false if user tries to follow themselves
   if(strcmp(user_name,list_of_profiles[userid].user_name) == 0 ){
      return 0;
   	}
	else{
		return 1;

   }
}

// IMPLEMENTAR ESSES DOIS
///////////////////////////////////////////////////apenas copiado,/////////////////////////
void sendhandler(notification *notif, packet msg, int userid, int sockfd){
 
   profile *p;
   int pending_notif_count;
   int num_followers = list_of_profiles[userid].follower_count;
   int id_notif;
   
   //Update notif id
   id_notif = list_of_profiles[userid].sent_notif_count;
   list_of_profiles[userid].sent_notif_count++;

   if(id_notif == MAX_NOTIFS){//Making it circular, will erase the first notification 
      id_notif = 0;           //if the server didnt send it (it should have by then)
   }

   //Create notification
   notif = malloc(sizeof(&notif));
   notif->id = id_notif;
   notif->sender = list_of_profiles[userid].user_name;
   notif->timestamp = msg.timestamp;
   notif->_string = (char*)malloc(strlen(msg._payload)*sizeof(char)+sizeof(char));
   memcpy(notif->_string,msg._payload,strlen(msg._payload)*sizeof(char)+sizeof(char));
   notif->length = msg.length;
   notif->pending = list_of_profiles[userid].follower_count;

   //Putting the notification on the current profile as send
   list_of_profiles[userid].list_send_notif[id_notif] = notif;

   //Putting the notification of followers pending list
   for(int i=0; i< num_followers;i++){

      p = list_of_profiles[userid].followed_users[i];

      pending_notif_count = p->pending_notif_count; 
      p->pending_notif_count++;

      if(p->pending_notif_count == MAX_NOTIFS){
         p->pending_notif_count =0;           
      }

      p->list_pending_notif[pending_notif_count].id_notif= id_notif;
      p->list_pending_notif[pending_notif_count].userid_notif= userid;
   
   }
 
}
//////////////////////////////////////////////////////////////////////////////////////

void followhandler(char *user_name, int userid, int sockfd){
   int followid;
   int followercount;

   followid = get_profile_id(list_of_profiles,user_name);

   //check if follower exists and is not already followed/is not the user
   if(!is_follow_valid(followid,userid,user_name,sockfd))
      return;
  
   //check if follower has not exceeded follower limit
   followercount =  list_of_profiles[followid].follower_count;
   if (followercount >= MAX_FOLLOW){
	   printf("User has reached max followers and could not follow %s", user_name);
   }
    
   //ADD FOLLOWER
   list_of_profiles[followid].follower_count++;
   list_of_profiles[followid].followed_users[followercount] =  &list_of_profiles[followid];

   printf("User %s just followed %s ", user_name );
 
}

void *clientmessagehandler(void *arg){
	thread_parameters *par = (thread_parameters*) arg;
	int userid = par->userid;
	int sockfd = par->sockfd;
	char newfollow[21];
	packet msg;
	notification *notif;

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
				sendhandler(notif, msg, userid, sockfd);
				pthread_mutex_unlock(&send_mutex);
				break;
			
			case FOLLOW:
				pthread_mutex_lock(&follow_mutex);
				strcpy(newfollow, msg._payload);
				followhandler(newfollow, userid, sockfd);
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

// handles client consumes (sending the notifications from their followers)
void *notificationhandler(void *arg){
	thread_parameters *par  = (thread_parameters*) arg;
	int sockfd = par->sockfd;
	int userid = par->userid;
	ids_notification notifid;
	profile *user = &list_of_profiles[userid];
	notification *notif;
	char* payload;
//using the same flag as other thread to maintain both synced up
	while(par->flag){
		//for each pending notification send message to user
		for(int i = 0; i < user->pending_notif_count; i++){
			notifid = user->list_pending_notif[i];
			//if notification exists then sends
			if (notifid.userid_notif != -1){
				//gets notification 
				notif = list_of_profiles[notifid.userid_notif].list_send_notif[notifid.id_notif];
				payload = malloc(notif->length+strlen(notif->sender) + 12*sizeof(char));
				//talvez o sprintf seja necessário pra colocar o valor na payload mas vamo ver

				//sends notification
				sendpacket(sockfd, NOTIFICATION, ++seqncount, sizeof(payload), getcurrenttime(),  payload, cli_addr);
				free(payload);

				//IMPLEMENTAR BARRIER

				//locks mutex to change list of notifications without having other thread take from it at the same time
				pthread_mutex_lock(&send_mutex);

				if(par->flag){
					//see if notification has not been deleted by another client of same user
					if(user->list_pending_notif[i].userid_notif != -1){

						notif->pending--;
						if(notif->pending == 0){
							//user no longer has notifications, notification is deleted
							notif = NULL;
						}

						//deletes notification from users list of pending notifications
						user->list_pending_notif[i].userid_notif = -1;
						user->list_pending_notif[i].id_notif = -1;
					}
				}
				//unlocks thread after removing notification from list, avoids conflicts between threads
				pthread_mutex_unlock(&send_mutex);

			}
		}
	}
	return;
}

int main(int argc, char *argv[])
{
	int one = 1;
	int sockfd, i; 
	socklen_t clilen;
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