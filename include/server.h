#ifndef __filesync_server__
#define __filesync_server__

#define MAX_NAME_LENGTH 50
#define MAX_PATH_LENGTH 255


#include "communication.h"

typedef struct client_info{
    char username[MAX_NAME_LENGTH];
    REMOTE_ADDR *client;
}CLIENT_INFO;


/** 
 * Escuta um cliente em um determinado socket 
 * */
void *listen_to_client(void *client_info);

/** 
 *  Cria um novo socket em uma nova thread
 */
int new_socket(CLIENT_INFO *client);

int upload(char *archive_name, char *archive_file, int data_socket);


#endif