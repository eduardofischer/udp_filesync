#include "../include/communication.h"

/* Inicializa um socket UDP*/
int create_udp_socket()
{
    int sockfd;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        printf("ERROR opening socket\n");
        return -1;
    }

    return sockfd;
}

/* Vincula um socket com um IP e uma porta */
int bind_udp_socket(int socket, char *ip, unsigned int port)
{
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;         // IPv4
    addr.sin_port = htons(port);       // Porta
    addr.sin_addr.s_addr = INADDR_ANY; // IP
    bzero(&(addr.sin_zero), 8);

    if (bind(socket, (struct sockaddr *)&addr, sizeof(struct sockaddr)) < 0)
    {
        printf("ERROR on binding\n");
        return -1;
    }

    return socket;
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
    if (n < 0)
        printf("ERROR sendto %d\n", errno);

    n = recvfrom(socket, &response, sizeof(PACKET), 0, (struct sockaddr *)&new_addr, &addr_len);

    if (n < 0 || response.header.type != ACK)
    {
        printf("ERROR recvfrom %d\n", errno);
        return -1;
    }

    return ntohs(new_addr.sin_port);
}

/** Envia um pacote de ACK */
int ack(int socket, struct sockaddr *cli_addr, socklen_t clilen){
    PACKET packet;
    int n;
    packet.header.type = ACK;

    n = sendto(socket, &packet, sizeof(PACKET), 0, (struct sockaddr *)cli_addr, clilen);

    return n;
}

/** 
 *  Inicia a comunicação de um cliente com o servidor 
 *  Retorna a porta com a qual o cliente deve se comunicar
 *  ou -1 em caso de erro
*/
int hello(int socket, REMOTE_ADDR addr, char *username){
    PACKET packet;
    int new_port;

    packet.header.type = HELLO;
    strcpy((char *)&(packet.data), username);

    new_port = send_packet(socket, addr, packet);

    if (new_port < 0){
        fprintf(stderr, "ERROR! Login failed\n");
        exit(0);
    }

    return new_port;
}

/** Escuta um cliente em um determinado socket */
void *listen_to_client(void *client){
    int new_socket, n;
    PACKET msg;
    REMOTE_ADDR addr = *(REMOTE_ADDR *) client;
    struct sockaddr_in cli_addr;
	socklen_t clilen = sizeof(cli_addr);

    new_socket = create_udp_socket();

    if(new_socket < 0){
        printf("ERROR creating new socket\n");
        exit(0);
    }

    cli_addr.sin_family = AF_INET;
    cli_addr.sin_port = htons(addr.port);
    cli_addr.sin_addr.s_addr = addr.ip;
    bzero(&(cli_addr.sin_zero), 8);

    n = ack(new_socket, (struct sockaddr *)&cli_addr, sizeof(struct sockaddr_in));
    if (n < 0){
        printf("Error ack %d/n", errno);
    }

    while(1){
		n = recvfrom(new_socket, &msg, PACKET_SIZE, 0, (struct sockaddr *) &cli_addr, &clilen);

		if (n < 0) 
			printf("ERROR on recvfrom");

		printf("    %s:%d > %s\n", inet_ntoa(*(struct in_addr *) &addr.ip), addr.port,(char *) &(msg.data));
		
		ack(new_socket, (struct sockaddr *) &cli_addr, clilen);
    }
}

/** 
 *  Cria um novo socket em uma nova thread
 *  Retorna a porta do novo socket ou -1 em caso de erro
 */
int new_socket(REMOTE_ADDR *client){
    pthread_t thr;       /* thread descriptor */

    pthread_create(&thr, NULL, listen_to_client, client); 

    return 0;
}