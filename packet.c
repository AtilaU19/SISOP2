#include "packet.h"

typedef struct __packet
{
    uint16_t type;      // Tipo do pacote (p.ex. DATA | CMD)
    uint16_t seqn;      // Número de sequência
    uint16_t length;    // Comprimento do payload
    uint16_t timestamp; // Timestamp do dado
    char *_payload;     // Dados da mensagem
} packet;

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
void sendpacket(int sockfd, int action, int seqn, int len, int timestamp, char *payload, struct sockaddr_in addr)
{
    packet msg;
    msg.type = action;
    msg.seqn = seqn;
    msg.length = len;
    msg.timestamp = timestamp;
    // printf("[+] DEBUG packet > Sending payload %s\n", payload);
    sendto(sockfd, &msg, 8, 0, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
    sendto(sockfd, payload, strlen(payload), 0, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
    // printf("%lu",sizeof(msg));
    //checkaddress(addr);
    //printf("[+] DEBUG packet > Sent message: %i, %i, %i, %i, %s\n", msg.type, msg.seqn, msg.length, msg.timestamp, payload);
}
// só recebe o packet, chamado pelo recvprintpacket
struct sockaddr_in recvpacket(int sockfd, packet *msg, struct sockaddr_in addr)
{
    socklen_t addr_size = sizeof(struct sockaddr_in);
    int n, recv = 10;
    // espera terminar de ler do socket
    // se a mensagem não for vazia executa a leitura
    // while(recvfrom(sockfd, (*msg)._payload, 8, 0, (struct sockaddr *) &addr, &addr_size)<8);
    // msg->_payload = (char*) malloc((msg->length)*sizeof(char));
    do
    {
        recv = recvfrom(sockfd, msg, 8, 0, (struct sockaddr *)&addr, &addr_size);
    }

    while (recv < 0);

    if (recv == 0)
    {
        printf("deu caca\n");
        return (addr);
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
    return (addr);
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
