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
    struct sockaddr_in dest_addr, new_addr;
    socklen_t addr_len = sizeof(new_addr);
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

	n = sendto(socket, &packet, PACKET_SIZE, 0, (struct sockaddr *) &dest_addr, sizeof(struct sockaddr_in));
	if (n < 0) 
		printf("ERROR sendto %d\n", errno);
	
	n = recvfrom(socket, &response, sizeof(PACKET), 0, (struct sockaddr *) &new_addr, &addr_len);
    
	if (n < 0 || response.header.type != ACK){
		printf("ERROR recvfrom %d\n", errno);
        return ERR_ACK;
    }

    return ntohs(new_addr.sin_port);
}

/** Envia um pacote de ACK */
int ack(int socket, const struct sockaddr *cli_addr, socklen_t clilen){
    PACKET packet;
    int n;
    packet.header.type = ACK;

    n = sendto(socket, &packet, sizeof(PACKET), 0, (struct sockaddr *) cli_addr, clilen);
    if(n<0){
        printf("Error sendto ack %d/n", errno);
    }

    return n;
}

int hello(int socket, REMOTE_ADDR addr, char *username){
    PACKET packet;
    int new_port;
 
    packet.header.type = HELLO;
    strcpy((char *) &(packet.data), username);

    new_port = send_packet(socket, addr, packet);

    if(new_port < 0){
        fprintf(stderr,"ERROR! Login failed\n");
        exit(0);
    }

    return new_port;
}