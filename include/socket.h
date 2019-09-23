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

/** Tamanho do datagrama */
#define DGRAM_SIZE 1024

/** Tipos de pacotes */
#define ERR     0xFF
#define HELLO   0x00
#define ACK     0x01
#define DATA    0x02
#define CMD     0x03

/** Estrutura do datagrama UDP */
typedef struct PacketHeader{
    char type;              // Tipo do pacote
    uint16_t seqn;          // Número de sequência
    uint32_t total_size;    // Número total de fragmentos
    uint16_t length;        // Comprimento do payload
} PACKET_HEADER;

#define DATA_LENGTH (DGRAM_SIZE - sizeof(PACKET_HEADER))

typedef struct PacketData{
    char data[DATA_LENGTH];   // Espaço restante do datagrama é preenchido com dados
} PACKET_DATA;

typedef struct Packet{
    PACKET_HEADER header;
    PACKET_DATA data;
} PACKET;

/** Estrutura com as informações do servidor */
typedef struct RemoteAddr{
    char *ip;
    uint16_t port;
} REMOTE_ADDR;

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
int send_message(int socket, REMOTE_ADDR addr, char *buffer, int length);

int hello(int socket, REMOTE_ADDR server, char *username);

#endif