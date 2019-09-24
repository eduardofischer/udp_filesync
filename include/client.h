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

int sendCommand(int socket, char* command);

void initDataPacketHeader(PACKET *toInit,uint32_t total_size, u_int16_t length);

int fileSize(FILE *toBeMeasured);

int isOpened(FILE *sourceFile);

int fileSizeInPackets(int fileSize);

#endif