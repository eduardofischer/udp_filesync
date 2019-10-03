#include "../include/communication.h"
#include "../include/server.h"

/**
 *  Inicializa um socket UDP
 * */
int create_udp_socket(){
    int sockfd;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        printf("ERROR opening socket\n");
        return -1;
    }

    return sockfd;
}

/** 
 *  Vincula um socket com um IP e uma porta
 * */
int bind_udp_socket(int socket, char *ip, unsigned int port){
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;         // IPv4
    addr.sin_port = htons(port);       // Porta
    addr.sin_addr.s_addr = INADDR_ANY; // IP
    bzero(&(addr.sin_zero), 8);

    if (bind(socket, (struct sockaddr *)&addr, sizeof(struct sockaddr)) < 0)
    {
        printf("ERROR on binding: %s\n", strerror(errno));
        return -1;
    }

    return socket;
}

uint16_t get_socket_port(int socket){
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    getsockname(socket, (struct sockaddr *) &addr, &addr_len);

    return ntohs(addr.sin_port);
}

/** 
 *  Envia uma mensagem e espera o ACK do destino
 *  Retorno: Porta do servidor
 *           -1 (Error)
 *           -2 (Time out)
 **/
int send_packet(int socket, REMOTE_ADDR addr, PACKET packet){
    struct sockaddr_in dest_addr, new_addr;
    socklen_t addr_len = sizeof(new_addr);
    int n;
    PACKET response;

    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(addr.port);
    dest_addr.sin_addr.s_addr = addr.ip;
    bzero(&(dest_addr.sin_zero), 8);

    n = sendto(socket, &packet, PACKET_SIZE, 0, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_in));
    if (n < 0){
        printf("ERROR send_packet sendto: %s\n", strerror(errno));
        return -1;
    }

    n = recvfrom(socket, &response, sizeof(PACKET), 0, (struct sockaddr *)&new_addr, &addr_len);
    if (n < 0 || response.header.type != ACK){
        printf("ERROR send_packet recvfrom %s\n", strerror(errno));
        return -1;
    }

    return ntohs(new_addr.sin_port);
}

int recv_packet(int socket, REMOTE_ADDR *addr, PACKET *packet){
    struct sockaddr_in new_addr;
    socklen_t addr_len = sizeof(new_addr);
    int n;

    n = recvfrom(socket, packet, sizeof(PACKET), 0, (struct sockaddr *)&new_addr, &addr_len);
    ack(socket, (struct sockaddr *)&new_addr, addr_len);

    if(addr != NULL){
        addr->ip = new_addr.sin_addr.s_addr;
        addr->port = ntohs(new_addr.sin_port);
    }

    return n;
}

/** 
 *  Envia um pacote de ACK 
 * */
int ack(int socket, struct sockaddr *cli_addr, socklen_t clilen){
    PACKET packet;
    int n;
    packet.header.type = ACK;

    n = sendto(socket, &packet, sizeof(PACKET), 0, (struct sockaddr *)cli_addr, clilen);

    return n;
}

/** 
 *  Envia um pacote de ERR
 * */
int err(int socket, struct sockaddr *cli_addr, socklen_t clilen, char *err_msg){
    PACKET packet;
    int n;
    packet.header.type = ERR;
    strcpy((char *)&(packet.data), err_msg);

    n = sendto(socket, &packet, sizeof(PACKET), 0, (struct sockaddr *)cli_addr, clilen);

    return n;
}

/** 
 *  Envia um comando genérico ao servidor e aguarda pelo ack do mesmo 
 * */
int send_command(int socket, REMOTE_ADDR server, char command, char* arg){
    PACKET packet;

    //Prepara o pacote de comando
    packet.header.type = CMD;
    packet.header.seqn = 0;
    packet.header.total_size = 1;
    packet.header.length = sizeof(COMMAND);

    (*(COMMAND *) &(packet.data)).code = command;
    if(arg != NULL){
        strcpy((*(COMMAND *) &(packet.data)).argument, arg);
    }else{
        strcpy((*(COMMAND *) &(packet.data)).argument, "");
    };
    //Envia o pacote
    return send_packet(socket, server, packet);
}

/**
 *  Inicializa o pacote de dados a ser enviado para o servidor. 
 * **/
void init_data_packet_header(PACKET *toInit, uint32_t total_size){
    toInit->header.type = DATA;
    toInit->header.seqn = 0;
    toInit->header.total_size = total_size;
}