#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include "../include/communication.h"

#define FILE_NAME_SIZE 255


/** Cria diretórios recursivamente */
int mkdir_recursive(char *dir);

/** Cria o diretório com o nome do usuário caso não exista */
int create_user_dir(char *user);

/** Cria o diretório local caso não exista */
int create_local_dir();

/**Retorna o tamanho do arquivo em bytes. **/
int fileSize(FILE *toBeMeasured);

/**Determina se o arquivo está aberto ou não. **/
int isOpened(FILE *sourceFile);

/**Determina o tamanho do arquivo em pacotes. **/
int fileSizeInPackets(int fileSize);

/**Escreve o pacote num arquivo, lidando com os ofsets dentro do arquivo. 
 * Deixa o ponteiro no ponto onde estava antes.
*/
int write_packet_to_the_file(PACKET *packet, FILE *file);

char **splitPath(char *name, int *size); 


