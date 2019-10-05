#ifndef __filesystem__
#define __filesystem__

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <string.h>
#include <time.h>
#include "../include/communication.h"

#define FILE_NAME_SIZE 255

#define LOCAL_DIR "./sync_dir/"
#define SERVER_DIR "./user_data/"
#define MAX_PATH_LENGTH 255

typedef struct DirEntry{
    char name[256];
    unsigned long int size;
    time_t last_modified;
    time_t last_status;
    time_t last_access;
} DIR_ENTRY;

/** Cria diretórios recursivamente */
int mkdir_recursive(char *dir);

/** Cria o diretório com o nome do usuário caso não exista */
int create_user_dir(char *user);

/** Cria o diretório local caso não exista */
int create_local_dir();

/** Retorna o tamanho do arquivo em bytes. **/
int fileSize(FILE *toBeMeasured);

/** Determina se o arquivo está aberto ou não. **/
int isOpened(FILE *sourceFile);

/** Determina o tamanho do arquivo em pacotes. **/
int fileSizeInPackets(int fileSize);


/**  Lê o diretório e preenche _entries_ com informações sobre os arquivos
 *  no diretório.
 *   Retorno: Número de entradas lidas
 *            -1 (ERRO)
*/
int get_dir_status(char *dir_path, DIR_ENTRY **entries);

void print_dir_status(DIR_ENTRY **entries, int n);

void compare_entry_diff(DIR_ENTRY *server_entries, DIR_ENTRY *client_entries, int n_server_ent, int n_client_ent, SYNC_LIST *list);

char **splitPath(char *name, int *size); 

#endif
