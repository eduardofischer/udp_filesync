#ifndef __filesync_server__
#define __filesync_server__

#include "communication.h"

/** 
 * Escuta um cliente em um determinado socket 
 * */
void *listen_to_client(void *client);

/** 
 *  Cria um novo socket em uma nova thread
 */
int new_socket(REMOTE_ADDR *client);


#endif