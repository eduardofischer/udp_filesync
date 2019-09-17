#ifndef __filesync__
#define __filesync__

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

#endif