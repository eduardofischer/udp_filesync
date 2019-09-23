#include <stdio.h>
#include <stdlib.h>
#include "../include/fsync_client.h"
#include "../include/socket.h"

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
    int socket, res;
    char buffer[256];
    REMOTE_ADDR server;

    if(argc < 2){
        fprintf(stderr, "ERROR! Invalid number of arguments.\n");
        exit(0);
    }

    server.ip = (char *)argv[1];
    server.port = PORT;

    if((socket = create_udp_socket()) < 0){
        fprintf(stderr,"ERROR opening socket\n");
        exit(0);
    }

	bzero(buffer, 256);
	strcpy(buffer, "Teste do socket UDP");

    res = send_message(socket, server, buffer, strlen(buffer));

    if(res == 0)
        printf("ACK Recebido!\n");
    else 
        printf("Erro! n=%d\n", res);
    
    return 0;
}
