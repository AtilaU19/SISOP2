#include <netdb.h>
#include <unistd.h>
#include <string.h>
 
 typedef struct __packet{
	uint16_t type; //Tipo do pacote (p.ex. DATA | CMD)
	uint16_t seqn; //Número de sequência
	uint16_t length; //Comprimento do payload 
	uint16_t timestamp; // Timestamp do dado
	char* _payload; //Dados da mensagem
 } packet;


//TEM QUE IMPLEMENTAR AINDA
void sendpacket(int sockfd, int action, int seqn, int len, int timestamp, char* payload){
	packet msg;
	msg.type = action;
	msg.seqn = seqn;
	msg.length = len;
	msg.timestamp = timestamp;

	write(sockfd, &msg, 8);
	write(sockfd,payload,strlen(payload));
}
//pega o packet recebido do recvpacket e printa caso tenha algum conteúdo
void recvprintpacket(int sockfd){
    packet msg;

    recvpacket(sockfd,&msg);

    if(msg._payload!=NULL && msg.length!=0){
        //POSSO ESTAR USANDO SPRINTF ERRADO
        sprintf("%s\n", msg._payload);
        free(msg._payload);
    }
}
// só recebe o packet, chamado pelo recvprintpacket
void recvpacket(int sockfd, packet* msg){
    //espera terminar de ler do socket
    while(read(sockfd,msg,8)<0);
    //se a mensagem não for vazia executa a leitura
    if((*msg).length!=0){
        (*msg)._payload = 
            (char*) malloc(
                ((*msg).length)*sizeof(char));

        read(sockfd, (*msg)._payload, (*msg).length);
        (*msg)._payload[(*msg).length-1]='\0';

    }
    else{
        (*msg)._payload = NULL;
    }
}
