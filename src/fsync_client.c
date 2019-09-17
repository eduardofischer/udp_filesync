#include <stdio.h>
#include <stdlib.h>
#include "../include/fsync_client.h"

/** Envia o arquivo para o servidor **/
int uploadFile(char* filePath){
    return -1;
};

/** Faz o download de um arquivo do servidor **/
int downloadFile(char *fileName){
    return -1;
};

/** Exclui um arquivo de sync_dir **/
int deleteFile(char* fileName){
    return -1;
};

/** Lista os arquivos salvos no servidor associados ao usuário **/
int listServer(){
    return -1;
};

/** Lista os arquivos salvos no diretório “sync_dir” **/
int listClient(){
    return -1;
};

/** Cria o diretório “sync_dir” e inicia as atividades de sincronização **/
int getSyncDir(){
    return -1;
};

/** Fecha a sessão com o servidor **/
int exitClient(){
    return -1;
};

int main(int argc, char const *argv[]){
    
    return 0;
}
