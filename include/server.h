#ifndef __filesync_server__
#define __filesync_server__

#include "communication.h"

typedef struct client_info{
    char username[MAX_NAME_LENGTH];
    REMOTE_ADDR client_addr;
} CLIENT_INFO;

typedef struct thread_info{
    CLIENT_INFO client;
    uint16_t sock_cmd;
    uint16_t sock_sync;
} THREAD_INFO;

typedef struct connection_info{
    CLIENT_INFO client;
    SERVER_PORTS_FOR_CLIENT ports;
} CONNECTION_INFO;

/** 
 * Escuta comandos do cliente em um determinado socket 
 * */
void *thread_client_cmd(void *client_info);

/** 
 * Sincroniza os arquivos com o cliente em outro socket
 * */
void *thread_client_sync(void *client_info);

/**
 * Servidor responde mensagem de HELLO enviando os novos sockets
 * com os quais o cliente deve se comunicar
 */
int hello(CONNECTION_INFO conn);

/** 
 *  Cria um novo socket em uma nova thread
 */
int new_client(CLIENT_INFO *client);

int upload(char *filename, char *user_dir, int socket);

int sync_user(int socket, char *user_dir, REMOTE_ADDR client_addr);


#endif