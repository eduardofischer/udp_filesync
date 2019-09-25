#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "../include/client.h"
#include "../include/communication.h"
#include "../include/filesystem.h"

#define COMMAND_SIZE 50 //TO-DO:Determinar o número correto

REMOTE_ADDR sessionAddress;

/** Envia o arquivo para o servidor **/
int uploadFile(char* filePath){
    int socketDataTransfer;
    int response;
    char command[COMMAND_SIZE] = "upload";
    FILE *sourceFile;
    int i;
    PACKET dataToTransfer;
    char buffer[DATA_LENGTH];


    socketDataTransfer = create_udp_socket();

    if (socketDataTransfer != ERR_SOCKET){
        response = sendCommand(socketDataTransfer,command);
        if(response >= 0){
             sourceFile = fopen(filePath,"rb");
             if(isOpened(sourceFile)){
                int sourceFileSize = fileSize(sourceFile); 
                int sourceFileSizeRemaining = sourceFileSize;
                int currentPacketLenght;
                initDataPacketHeader(&dataToTransfer,(u_int32_t)fileSizeInPackets(sourceFileSize));
                
                for(i = 0; i < fileSizeInPackets(sourceFileSize); i++){
                    //Preenchimento do pacote: dados e cabeçalho
                    currentPacketLenght = (sourceFileSizeRemaining / DATA_LENGTH) >= 1? DATA_LENGTH : sourceFileSizeRemaining % DATA_LENGTH;
                    fread(buffer,sizeof(char),DATA_LENGTH,sourceFile);
                    memcpy(dataToTransfer.data.data,buffer,currentPacketLenght);
                    dataToTransfer.header.seqn = i;
                    dataToTransfer.header.length = currentPacketLenght;
                    
                    //Enquanto o servidor não recebeu o pacote, tenta re-enviar pra ele
                    while(send_packet(socketDataTransfer,sessionAddress,dataToTransfer) < 0);
                    
                    //Tamanho restante a ser transmitido
                    sourceFileSizeRemaining -= currentPacketLenght;
                }
                return SUCCESS;
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
    return send_packet(socket, sessionAddress, toSend);
}

void initDataPacketHeader(PACKET *toInit,uint32_t total_size){
    toInit->header.type = DATA;
    toInit->header.seqn = 0;
    toInit->header.total_size = total_size;
}

int fileSize(FILE *toBeMeasured){
    int size;
    
    //Ao infinito e além para medir o tamanho do arquivo.
    fseek(toBeMeasured, 0L, SEEK_END);
    size = ftell(toBeMeasured);
    //Retorna ao inicio depois da longa viagem.
    rewind(toBeMeasured);
    return size;
}

int isOpened(FILE *sourceFile){
    return sourceFile != NULL;
}

int fileSizeInPackets(int fileSize){
    int totalPackets;
    //O tamanho do arquivo em pacotes é acrescido de um pacote caso haja resto.
    totalPackets = fileSize % DATA_LENGTH? (fileSize / DATA_LENGTH) : (fileSize / DATA_LENGTH + 1);
    return totalPackets;
}

int main(int argc, char const *argv[]){
    int socket, res;
    REMOTE_ADDR server;
    char username[64];
    struct hostent *client_host;
    PACKET msg;

    if(argc < 3){
        fprintf(stderr, "ERROR! Invalid number of arguments.\n");
        fprintf(stderr, "Command should be in the form: client <username> <server_ip_address>\n");
        exit(0);
    }

    if ((client_host = gethostbyname((char *)argv[2])) == NULL){
        fprintf(stderr, "ERROR! No such host\n");
        exit(0);
    }
    
	if (create_local_dir() < 0) 
		exit(0);

    strcpy((char *) username, argv[1]);
    server.ip = *(unsigned long *) client_host->h_addr;
    server.port = PORT;

    if((socket = create_udp_socket()) < 0){
        fprintf(stderr,"ERROR opening socket\n");
        exit(0);
    }

    // Conecta com o servidor e atualiza a porta
    server.port = hello(socket, server, username);

    printf("📡 Client connected to %s:%d\n\n", inet_ntoa(*(struct in_addr *) &server.ip), server.port);

    // Enviando mensagem de teste
	strcpy((char *) &(msg.data), "Teste do socket UDP");
    res = send_packet(socket, server, msg);
    if(res < 0)
        printf("Erro! n=%d\n", res); 

    //CLI
    printf("Available commands:\n\n\tupload <path/filename.ext>\n\tdownload <filename.ext>\n\tdelete <filename.ext>\n\tlist_server\n\tlist_client\n\tget_sync_dir\n\texit\n\n");

    char user_input[COMMAND_SIZE];
    char *user_cmd;
    char *user_arg;

    int session_alive = 1;
    do{
        printf("udp_filesync > ");
        fgets(user_input, COMMAND_SIZE, stdin);

        user_cmd = strtok(user_input, " ");
        user_arg = strtok(NULL, " ");

        if (!strcmp(user_cmd,"upload")) {
            if (uploadFile(user_arg) == -1){
                printf("Error uploading file.\n");
            }
        }else if(!strcmp(user_cmd, "download")) {
           if (downloadFile(user_arg) == -1){
                printf("Error downloading file.\n");
            }
        }else if(!strcmp(user_cmd, "delete")) {
            if (deleteFile(user_arg) == -1){
                printf("Error deleting file.\n");
            }
        }else if(!strcmp(user_cmd, "list_server\n")){
            if (listServer() == -1){
                printf("Error listing server files.\n");
            }
        }else if(!strcmp(user_cmd, "list_client\n")){
            if (listClient() == -1){
                printf("Error listing client files.\n");
            }
        }else if(!strcmp(user_cmd, "get_sync_dir\n")){
            if (getSyncDir() == -1){
                printf("Error getting sync directory.\n");
            }
        }else if(!strcmp(user_input, "exit\n")){
            if (exitClient() == -1){
                printf("Error exiting client.\n");
            }else{
                session_alive = 0;
            }
        }
    } while(session_alive);
    return 0;
}
