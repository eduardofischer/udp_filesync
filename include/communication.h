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
#include <math.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/stat.h>


#define MAX_NAME_LENGTH 50

#define PORT 4000

/**Erros */
#define ERR_SOCKET -1
#define ERR_ACK -2
#define ERR_OPEN_FILE -3
#define ERR_WRITING_FILE -4

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


typedef struct FileInfo{
    time_t modification_time;
    time_t access_time;
    char filename[MAX_NAME_LENGTH];
} FILE_INFO;

/** Estrutura do datagrama UDP */
typedef struct PacketHeader{
    char type;              // Tipo do pacote
    uint16_t seqn;          // Número de sequência
    uint32_t total_size;    // Número total de fragmentos
    uint16_t length;        // Comprimento do payload
} PACKET_HEADER;

#define DATA_LENGTH (PACKET_SIZE - sizeof(PACKET_HEADER))

#define MAX_NAME_LENGTH 50

/** Estrutura com as informações do servidor */
typedef struct RemoteAddr{
    unsigned long ip; // load with inet_aton()
    uint16_t port;
} REMOTE_ADDR;

typedef struct Packet{
    PACKET_HEADER header;
    char data[DATA_LENGTH];
} PACKET;

/** Estrutura do comando **/
typedef struct command{
    char code;
    char argument[DATA_LENGTH - 1]; // Espaço restante do datagrama é preenchido com dados
} COMMAND;

typedef struct server_ports{
    uint16_t port_cmd;
    uint16_t port_sync;
} SERVER_PORTS_FOR_CLIENT;

/** Lista de sincronização */
typedef struct sync_list{
    int n_downloads;
    int n_uploads;
    char *list;
} SYNC_LIST;

/** Inicializa um socket UDP */
int create_udp_socket();

/** Vincula um socket com um IP e uma porta */
int bind_udp_socket(int socket, char *ip, unsigned int port);

uint16_t get_socket_port(int socket);

/** 
 *  Envia uma mensagem e espera o ACK do destino
 *  Retorno: Porta do servidor
 *           -1 (Error)
 *           -2 (Time out)
 **/ 
int send_packet(int socket, REMOTE_ADDR addr, PACKET packet);

int recv_packet(int socket, REMOTE_ADDR *addr, PACKET *packet);

/** Envia um pacote de ACK */
int ack(int socket, struct sockaddr *cli_addr, socklen_t clilen);

/** 
 *  Envia um pacote de ERR
 * */
int err(int socket, struct sockaddr *cli_addr, socklen_t clilen, char *err_msg);

/** 
 *  Envia um comando genérico ao servidor e aguarda pelo ack do mesmo 
 * */
int send_command(int socket, REMOTE_ADDR server, char command, char* arg);

/** 
 *  Envia um comando de upload ao servidor e aguarda pelo ack do mesmo 
 * */
int send_upload(int socket, REMOTE_ADDR server, FILE_INFO *file_info);

/**
 *  Inicializa o pacote de dados a ser enviado para o servidor. 
 * **/
void init_data_packet_header(PACKET *toInit,uint32_t total_size);

int send_file(REMOTE_ADDR address, char *filePath);

int receive_file(FILE_INFO file_info, char *dir_path, int dataSocket);

/**Escreve o pacote num arquivo, lidando com os ofsets dentro do arquivo. 
 * Deixa o ponteiro no ponto onde estava antes.
*/
int write_packet_to_the_file(PACKET *packet, FILE *file);

#endif