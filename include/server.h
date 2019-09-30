#ifndef __filesync_server__
#define __filesync_server__

#include "communication.h"

#define MAX_NAME_LENGTH 50

typedef struct client_info{
    char username[MAX_NAME_LENGTH];
    REMOTE_ADDR client_addr;
}CLIENT_INFO;

/** 
 * Escuta um cliente em um determinado socket 
 * */
void *listen_to_client(void *client_info);

/** 
 *  Cria um novo socket em uma nova thread
 */
int new_socket(CLIENT_INFO *client);

int upload(char *filename, char *user_dir, int socket);

int sync_user(int socket, char *user_dir);


#endif