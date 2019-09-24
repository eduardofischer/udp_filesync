#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "../include/client.h"
#include "../include/communication.h"

#define COMMAND_SIZE 25 //TO-DO:Determinar o número correto

/** Envia o arquivo para o servidor **/
int uploadFile(char* filePath){
    int socketDataTransfer;
    int response;
    char command[COMMAND_SIZE] = "upload";
    FILE *sourceFile;
    int bytesRead;
    int i;
    PACKET dataToTransfer;
    char buffer[DATA_LENGTH];


    socketDataTransfer = create_udp_socket();

    if (socketDataTransfer != ERR_SOCKET){
        response = sendCommand(socketDataTransfer,command);
        if(response == SUCCESS){
             sourceFile = fopen(filePath,"rb");
             if(isOpened(sourceFile)){
                int sourceFileSize = fileSize(sourceFile); 
                
                for(i = 0; i < fileSizeInPackets(sourceFileSize); i++){
                    fread(buffer,sizeof(char),DATA_LENGTH,sourceFile);


                }
             }
             return ERR_OPEN_FILE;
        } 
        else{
            printf("Server didn't return ack (busy?)");
            return ERR_ACK;
        }
    }

    return ERR_SOCKET;

    
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

/**Envia um comando genérico ao servidor e aguarda pelo ack do mesmo*/
int sendCommand(int socket, char* command){
    PACKET toSend;
    //Prepara o pacote de comando
    toSend.header.type = CMD;
    toSend.header.seqn = 0;
    toSend.header.length = strlen(command);  
    toSend.header.total_size = 1;
    memcpy(toSend.data.data, command, sizeof(char) * toSend.header.length);
    
    //Envia o pacote
    return send_packet(socket,sessionAddress,toSend);
}

int initDataPacketHeader(PACKET *toInit,uint32_t total_size, u_int16_t length){
    toInit->header.type = DATA;
    toInit->header.seqn = 0;
}

int fileSize(FILE *toBeMeasured){
    int size;
    
    fseek(toBeMeasured, 0L, SEEK_END);
    size = ftell(toBeMeasured);
    return size;
}

int isOpened(FILE *sourceFile){
    return sourceFile != NULL;
}

int fileSizeInPackets(int fileSize){
    int totalPackets;
    totalPackets = fileSize % DATA_LENGTH? (fileSize / DATA_LENGTH) : (fileSize / DATA_LENGTH + 1);
    return totalPackets;
}

int main(int argc, char const *argv[]){
    int socket, res;
    char buffer[256];
    REMOTE_ADDR server;
    PACKET msg;

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
	strcpy((char *) &(msg.data), "Teste do socket UDP");

    res = send_packet(socket, server, msg);

    if(res == 0)
        printf("ACK Recebido!\n");
    else 
        printf("Erro! n=%d\n", res);
    
    return 0;
}
