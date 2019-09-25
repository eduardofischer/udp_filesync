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
#include <pthread.h> 

#define PORT 4000

/**Erros */
#define ERR_SOCKET -1
#define ERR_ACK -2
#define ERR_OPEN_FILE -3

/** Códigos de Retorno */
#define SUCCESS 0

/** Tamanho do datagrama */
#define PACKET_SIZE 1024

/** Tipos de pacotes */
#define ERR     0xFF
#define HELLO   0x00
#define ACK     0x01
#define DATA    0x02
#define CMD     0x03

/** Codigos dos comandos **/
#define UPLOAD   0x04
#define DOWNLOAD 0x05
#define DELETE   0x06
#define LST_SV   0x07
#define LST_CLI  0x08
#define SYNC_DIR 0x09
#define EXIT     0x10

/** Estrutura do comando **/
typedef struct command{
    char code;
    char *arguments;
} COMMAND;

/** Estrutura do datagrama UDP */
typedef struct PacketHeader{
    char type;              // Tipo do pacote
    uint16_t seqn;          // Número de sequência
    uint32_t total_size;    // Número total de fragmentos
    uint16_t length;        // Comprimento do payload
} PACKET_HEADER;

#define DATA_LENGTH (PACKET_SIZE - sizeof(PACKET_HEADER))

typedef struct PacketData{
    char data[DATA_LENGTH];   // Espaço restante do datagrama é preenchido com dados
} PACKET_DATA;

/** Estrutura com as informações do servidor */
typedef struct RemoteAddr{
    unsigned long ip; // load with inet_aton()
    uint16_t port;
} REMOTE_ADDR;

typedef struct Packet{
    PACKET_HEADER header;
    PACKET_DATA data;
} PACKET;

/** Inicializa um socket UDP */
int create_udp_socket();

/** Vincula um socket com um IP e uma porta */
int bind_udp_socket(int socket, char *ip, unsigned int port);

/** 
 *  Envia uma mensagem e espera o ACK do destino
 *  Retorno: Porta do servidor
 *           -1 (Error)
 *           -2 (Time out)
 **/ 
int send_packet(int socket, REMOTE_ADDR addr, PACKET packet);

/** Envia um pacote de ACK */
int ack(int socket, struct sockaddr *cli_addr, socklen_t clilen);

/** 
 *  Inicia a comunicação de um cliente com o servidor 
 *  Retorna a porta com a qual o cliente deve se comunicar
 *  ou -1 em caso de erro
*/
int hello(int socket, REMOTE_ADDR addr, char *username);

/** 
 * Escuta um cliente em um determinado socket 
 * */
void *listen_to_client(void *client);

/** 
 *  Cria um novo socket em uma nova thread
 */
int new_socket(REMOTE_ADDR *client);

#endif