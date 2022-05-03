#include "packet.h"

typedef struct __packet
{
    uint16_t type;      // Tipo do pacote (p.ex. DATA | CMD)
    uint16_t seqn;      // Número de sequência
    uint16_t length;    // Comprimento do payload
    uint16_t timestamp; // Timestamp do dado
    uint16_t userid;
    char *_payload;     // Dados da mensagem
} packet;

typedef struct __packetReturn
{
    struct sockaddr_in addr;
    int success;
} packetReturn;


// pega o tempo no momento da chamada
int getcurrenttime()
{
    time_t currenttime = time(NULL);
    struct tm *structtm = localtime(&currenttime);
    int hr = (*structtm).tm_hour;
    int min = (*structtm).tm_min;

    return hr * 100 + min;
}

void checkaddress(struct sockaddr_in addr){
    
    char buffer[INET_ADDRSTRLEN];
    inet_ntop( AF_INET, &addr.sin_addr, buffer, sizeof(buffer));
    printf("received address: %s\n", buffer);
}

// Envia packet para endereço addr no socket sockfd
int sendpacket(int sockfd, int action, int seqn, int len, int timestamp, char *payload, struct sockaddr_in addr)
{   
    int err = 0;
    socklen_t size = sizeof(int);
    int is_connected = getsockopt (sockfd, SOL_SOCKET, SO_ERROR, &err, &size);
    if (is_connected == 0 && action >=0 && action <= MAX_TYPE){
        packet msg;
        msg.type = action;
        msg.seqn = seqn;
        msg.length = len;
        msg.timestamp = timestamp;
        msg.userid = -1;
        // printf("[+] DEBUG packet > Sending payload %s\n", payload);
        sendto(sockfd, &msg, 8, 0, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
        sendto(sockfd, payload, strlen(payload), 0, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
        // printf("%lu",sizeof(msg));
        //checkaddress(addr);
        //printf("[+] DEBUG packet > Sent message: %i, %i, %i, %i, %s\n", msg.type, msg.seqn, msg.length, msg.timestamp, payload);
        return 1;
    }
    return 0;
}

void sendpacket(int sockfd, int userid, int action, int seqn, int len, int timestamp, char *payload, struct sockaddr_in addr)
{
    packet msg;
    msg.type = action;
    msg.seqn = seqn;
    msg.length = len;
    msg.timestamp = timestamp;
    msg.userid = userid;
    // printf("[+] DEBUG packet > Sending payload %s\n", payload);
    sendto(sockfd, &msg, 8, 0, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
    sendto(sockfd, payload, strlen(payload), 0, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
    // printf("%lu",sizeof(msg));
    //checkaddress(addr);
    //printf("[+] DEBUG packet > Sent message: %i, %i, %i, %i, %s\n", msg.type, msg.seqn, msg.length, msg.timestamp, payload);
}

void sendpacketwithid(int sockfd, int userid, int action, int seqn, int len, int timestamp, char *payload, struct sockaddr_in addr)     //a principio seria igual a de cima mas com o userid a mais
{        
    packet msg;
    msg.type = action;
    msg.seqn = seqn;
    msg.length = len;
    msg.timestamp = timestamp;
    msg.userid = userid;
    sendto(sockfd, &msg, 8, 0, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
    sendto(sockfd, payload, strlen(payload), 0, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
    
}

// só recebe o packet, chamado pelo recvprintpacket
packetReturn recvpacket(int sockfd, packet *msg, struct sockaddr_in addr)
{
    socklen_t addr_size = sizeof(struct sockaddr_in);
    int n, recv = 10;
    packetReturn pr;
    pr.addr = addr;
    pr.success = 0;
    int succeeded, err = 0;
    socklen_t size = sizeof(int);

    int is_connected = getsockopt (sockfd, SOL_SOCKET, SO_ERROR, &err, &size);
    // espera terminar de ler do socket
    // se a mensagem não for vazia executa a leitura
    // while(recvfrom(sockfd, (*msg)._payload, 8, 0, (struct sockaddr *) &addr, &addr_size)<8);
    // msg->_payload = (char*) malloc((msg->length)*sizeof(char));
    if(is_connected == 0){
        do
        {
            recv = recvfrom(sockfd, msg, 8, 0, (struct sockaddr *)&addr, &addr_size);
        }

        while (recv < 0);

        if (recv == 0)
        {   
            pr.addr = addr;
            printf("deu caca\n");
            return (pr);
        }

        // msg->_payload[msg->length-1]='\0';
        if (msg->length != 0)
        {
            // printf("[+] DEBUG packet > received %s\n", msg->_payload);
            msg->_payload = (char *)malloc((msg->length) * sizeof(char));
            n = recvfrom(sockfd, msg->_payload, msg->length /*+ strlen(msg->_payload)*/, 0, (struct sockaddr *)&addr, &addr_size);
            msg->_payload[msg->length - 1] = '\0';
        }
        else
        {
            printf("Empty message");
            // msg->_payload=NULL;
        }
        // printf("ADDRESS IN RECVPACKET %s\n", addr);
        //printf("[+] DEBUG packet > Received message: %i, %i, %i, %i, %s\n", msg->type, msg->seqn, msg->length, msg->timestamp, msg->_payload);
        pr.addr = addr;
        pr.success = 1;

        return (pr);
    }
    return (pr);
}
// pega o packet recebido do recvpacket e printa caso tenha algum conteúdo
void recvprintpacket(int sockfd, struct sockaddr_in addr)
{
    packet msg;

    recvpacket(sockfd, &msg, addr);

    if (msg._payload != NULL && msg.length != 0)
    {
        printf("%s\n", msg._payload);
        free(msg._payload);
    }
}
