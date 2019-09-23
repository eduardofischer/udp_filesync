#include <stdio.h>
#include <stdlib.h>
#include "../include/communication.h"

/* Inicializa um socket UDP*/
int create_udp_socket(){
    int sockfd;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
        printf("ERROR opening socket\n");
        return -1;
    }

    return sockfd;
}

/* Vincula um socket com um IP e uma porta */
int bind_udp_socket(int socket, char *ip, unsigned int port){
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;   // IPv4
	addr.sin_port = htons(port); // Porta
	addr.sin_addr.s_addr = INADDR_ANY; // IP
	bzero(&(addr.sin_zero), 8);

    if (bind(socket, (struct sockaddr *) &addr, sizeof(struct sockaddr)) < 0){
        printf("ERROR on binding\n");
        return -1;
    }

    return socket;
}

/* Envia uma mensagem */
int send_packet(int socket, REMOTE_ADDR addr, PACKET packet){
    struct sockaddr_in dest_addr;
    struct hostent *server;
    int n;
    PACKET response;

    if((server = gethostbyname(addr.ip)) == NULL) {
        fprintf(stderr,"ERROR! No such host\n");
        exit(0);
    }	

    dest_addr.sin_family = AF_INET;     
	dest_addr.sin_port = htons(addr.port);    
	dest_addr.sin_addr = *((struct in_addr *)server->h_addr);
	bzero(&(dest_addr.sin_zero), 8);  

	n = sendto(socket, &packet, PACKET_SIZE, 0, (const struct sockaddr *) &dest_addr, sizeof(struct sockaddr_in));
	if (n < 0) 
		printf("ERROR sendto %d\n", errno);
	
	n = recvfrom(socket, &response, sizeof(PACKET), 0, NULL, 0);
    
	if (n < 0 || response.header.type != ACK){
		printf("ERROR recvfrom %d\n", errno);
        return -1;
    }

    return 0;
}

/** Envia um pacote de ACK */
int ack(int socket, const struct sockaddr *cli_addr, socklen_t clilen){
    PACKET packet;
    int n;
    packet.header.type = ACK;

    n = sendto(socket, &packet, sizeof(PACKET), 0, (const struct sockaddr *) cli_addr, clilen);
    if(n<0){
        printf("Error sendto %d/n", errno);
    }

    return n;
}