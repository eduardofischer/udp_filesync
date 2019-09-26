#ifndef __filesync_client__
#define __filesync_client__

#include "communication.h"

#define COMMAND_SIZE 256

/** Envia o arquivo para o servidor **/
int uploadFile(char* filePath);

/** Faz o download de um arquivo do servidor **/
int downloadFile(char *fileName);

/** Exclui um arquivo de sync_dir **/
int deleteFile(char* fileName);

/** Lista os arquivos salvos no servidor associados ao usuário **/
int listServer();

/** Lista os arquivos salvos no diretório “sync_dir” **/
int listClient();

/** Cria o diretório “sync_dir” e inicia as atividades de sincronização **/
int getSyncDir();

/** Fecha a sessão com o servidor **/
int exit_client();

/**Retorna o tamanho do arquivo em bytes. **/
int fileSize(FILE *toBeMeasured);

/**Determina se o arquivo está aberto ou não. **/
int isOpened(FILE *sourceFile);

/**Determina o tamanho do arquivo em pacotes. **/
int fileSizeInPackets(int fileSize);

/** Imprime as opções da CLI */
void print_cli_options();

/** Exibe uma interface para entrada de comandos para o usuário */
void run_cli();

#endif