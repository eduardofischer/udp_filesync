#ifndef __filesync_client__
#define __filesync_client__

#include "communication.h"

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
int exitClient();

/**Envia um pacote de comando para o servidor. TODO: Trocar por struct**/
int sendCommand(int socket, char* command);

/**Inicializa o pacote de dados a ser enviado para o servidor. **/
void initDataPacketHeader(PACKET *toInit,uint32_t total_size);

/**Retorna o tamanho do arquivo em bytes. **/
int fileSize(FILE *toBeMeasured);

/**Determina se o arquivo está aberto ou não. **/
int isOpened(FILE *sourceFile);

/**Determina o tamanho do arquivo em pacotes. **/
int fileSizeInPackets(int fileSize);

#endif