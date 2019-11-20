#ifndef __filesync_server__
#define __filesync_server__

#define MAX_NAME_LENGTH 50
#define MAX_PATH_LENGTH 255

#define NUM_OF_MAX_CONNECTIONS 255

#include "communication.h"

typedef struct client_mutex_info{
    pthread_mutex_t sync_or_command;
    int clients_connected;
} CLIENT_MUTEX;

typedef struct client_info{
    char username[MAX_NAME_LENGTH];
    REMOTE_ADDR client_addr;
} CLIENT_INFO;

typedef struct thread_info{
    CLIENT_INFO client;
    uint16_t sock_cmd;
    uint16_t sock_sync;
    pthread_t tid_sync;
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

int sync_user(int socket, char *user_dir, REMOTE_ADDR client_addr);

int list_server(int socket, char *user_dir, REMOTE_ADDR client_addr);

int delete(char *file_name, char *client_folder_path);

void *thread_backup_cmd(void *thread_info);

int new_backup(CLIENT_INFO* backup_info);

#endif