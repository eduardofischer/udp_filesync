#include <stdio.h>
#include <stdlib.h>
#include "../include/client.h"
#include "../include/communication.h"

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
    REMOTE_ADDR server;
    char username[64];
    PACKET msg;

    if(argc < 3){
        fprintf(stderr, "ERROR! Invalid number of arguments.\n");
        fprintf(stderr, "Command should be in the form: client <username> <server_ip_address>\n");
        exit(0);
    }

    strcpy((char *) username, argv[1]);
    server.ip = (char *)argv[2];
    server.port = PORT;

    if((socket = create_udp_socket()) < 0){
        fprintf(stderr,"ERROR opening socket\n");
        exit(0);
    }

    // Conecta com o servidor e atualiza a porta
    server.port = hello(socket, server, username);

    printf("Client connected to %s:%d\n\n", server.ip, server.port);

    // Enviando mensagem de teste
	strcpy((char *) &(msg.data), "Teste do socket UDP");
    res = send_packet(socket, server, msg);
    if(res < 0)
        printf("Erro! n=%d\n", res); 
         
    return 0;
}
