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

typedef struct mutex_and_backup_info{
    CLIENT_MUTEX client_mutex;
    REMOTE_ADDR *backup_addresses;
}CLIENT_MUTEX_AND_BACKUP;

typedef struct thread_info{
    CLIENT_INFO client;
    uint16_t sock_cmd;
    uint16_t sock_sync;
    pthread_t tid_sync;
} THREAD_INFO;

/** 
 * Escuta comandos do cliente em um determinado socket 
 * */
void *thread_client_cmd(void *client_info);

/** 
 * Sincroniza os arquivos com o cliente em outro socket
 * */
void *thread_client_sync(void *client_info);

/** 
 *  Cria um novo socket em uma nova thread
 */
int new_client(CLIENT_INFO *client);

int sync_user(int socket, char *user_dir, REMOTE_ADDR client_addr);

int list_server(int socket, char *user_dir, REMOTE_ADDR client_addr);

int delete(char *file_name, char *client_folder_path);

void *thread_backup_cmd(void *thread_info);

int new_backup(CLIENT_INFO* backup_info);

/*	DEVE SER CHAMADA APENAS QUANDO UM NOVO SERVER ASSUME.
	Desaloca memória associada a lista de devices e seta o número para 0.
*/
void resetDevicesList();

#endif