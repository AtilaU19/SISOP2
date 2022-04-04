#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
 
 typedef struct __packet{
	uint16_t type; //Tipo do pacote (p.ex. DATA | CMD)
	uint16_t seqn; //Número de sequência
	uint16_t length; //Comprimento do payload 
	uint16_t timestamp; // Timestamp do dado
	char* _payload; //Dados da mensagem
 } packet;

//pega o tempo no momento da chamada
int getcurrenttime(){
	time_t currenttime = time(NULL);
	struct tm *structtm = localtime(&currenttime);
	int hr = (*structtm).tm_hour;
	int min = (*structtm).tm_min;

	return hr*100+min;
}
//TEM QUE IMPLEMENTAR AINDA
void sendpacket(int sockfd, int action, int seqn, int len, int timestamp, char* payload, struct sockaddr_in serv_addr){
    
    packet msg;
	msg.type = action;
	msg.seqn = seqn;
	msg.length = len;
	msg.timestamp = timestamp;

	sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in));
    printf("Message data %i, %i, %i, %i, %s.\n", msg.type, msg.seqn, msg.length, msg.timestamp, payload);;
}
// só recebe o packet, chamado pelo recvprintpacket
void recvpacket(int sockfd, packet* msg, struct sockaddr_in serv_addr){
    socklen_t addr_size = sizeof(serv_addr);
    //espera terminar de ler do socket
    while(read(sockfd,msg,8)<0);
    //se a mensagem não for vazia executa a leitura
    if((*msg).length!=0){
        (*msg)._payload = 
            (char*) malloc(
                ((*msg).length)*sizeof(char));

        recvfrom(sockfd, (*msg)._payload, sizeof((*msg)._payload), 0, (struct sockaddr *) &serv_addr, &addr_size);
        printf("[+] Data received: %i, %i, %i, %i, %s\n",msg->type, msg->seqn, msg->length, msg->timestamp, msg->_payload);

    }
    else{
        (*msg)._payload = NULL;
    }
}
//pega o packet recebido do recvpacket e printa caso tenha algum conteúdo
void recvprintpacket(int sockfd, struct sockaddr_in serv_addr){
    packet msg;

    recvpacket(sockfd,&msg, serv_addr);

    if(msg._payload!=NULL && msg.length!=0){
        //POSSO ESTAR USANDO SPRINTF ERRADO
        printf("%s\n", msg._payload);
        free(msg._payload);
    }
}

