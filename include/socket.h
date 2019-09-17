#ifndef __socket__
#define __socket__

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

// #define PORT 3000

typedef struct Distance{
    int feet;
    float inch;
} distances;

/** Inicializa um socket UDP */
int create_udp_socket();

/** Vincula um socket com um IP e uma porta */
int bind_udp_socket(int socket, char *ip, unsigned int port);

/** 
 *  Envia uma mensagem e espera o ACK do destino
 *  Return:  0 (Ok!)
 *          -1 (Error)
 *          -2 (Time out)
 **/ 
int send_message(int socket, char *ip, unsigned int port, char *buffer, int length);

#endif