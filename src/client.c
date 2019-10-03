#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "../include/client.h"
#include "../include/communication.h"
#include "../include/filesystem.h"
    
int socketfd;
REMOTE_ADDR server;

/** Envia o arquivo para o servidor **/
int uploadFile(char* filePath){
    return send_file(server,filePath);
}

/** Faz o download de um arquivo do servidor **/
int downloadFile(int socket,char *filePath){
    int n;
    PACKET msg;
    struct sockaddr_in cli_addr;
	socklen_t clilen = sizeof(cli_addr);
    FILE_INFO file_info;
    COMMAND *cmd;

    //Envia um comando de download
    send_command(socket,server,DOWNLOAD,filePath);
    //Espera pelas mensagens contendo as informações do arquivo (tempos)
    do{
        n = recvfrom(socket, &msg, PACKET_SIZE, 0, (struct sockaddr *) &cli_addr, &clilen);
    }while(n < 0);
    ack(socket,(struct sockaddr*)&cli_addr, clilen);
    //Após recebida a mensagem acessa as informações do arquivo
    cmd = (COMMAND *) &msg.data;
    file_info = *((FILE_INFO*)cmd->argument);
    printf("%s",file_info.filename);
    //Recebe o arquivo informado em file_info.
    return receive_file(file_info, ".", socket);
}

/** Exclui um arquivo de sync_dir **/
int deleteFile(char* fileName){
    int socketDataTransfer;
    int response;

    socketDataTransfer = create_udp_socket();

    if (socketDataTransfer != ERR_SOCKET){
        response = send_command(socketDataTransfer, server, DELETE, fileName);
        
        if(response >= 0){
            return SUCCESS;
        }
        else{
            printf("Server didn't return ack (busy?)\n");
            return ERR_ACK;
        }
    }
    else{
        return ERR_SOCKET; 
    };
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
int exit_client(){
    return send_command(socketfd, server, EXIT, NULL);
};

void print_cli_options(){
    printf("Available commands:\n\n");
    printf("\t📤  upload <path/filename.ext>\n");
    printf("\t📥  download <filename.ext>\n");
    printf("\t❌  delete <filename.ext>\n");
    printf("\t📃  list_server\n");
    printf("\t📃  list_client\n");
    printf("\t📁  get_sync_dir\n");
    printf("\t🏃  exit\n\n");
}

void run_cli(int socket){
    print_cli_options();

    char user_input[COMMAND_SIZE];
    char *user_cmd;
    char *user_arg;

    int session_alive = 1;
    do{
        printf("udp_filesync > ");
        fgets(user_input, COMMAND_SIZE, stdin);

        user_cmd = strtok(user_input, " ");
        user_arg = strtok(NULL, " \n");

        if(user_arg != NULL && strlen(user_arg) > FILE_NAME_SIZE){
            printf("Filename too long, max is 255 characters. Please enter your command again.");
            run_cli(socket);
            exit(0);
        }
        
        if (!strcmp(user_cmd,"upload")) {
            if (uploadFile(user_arg) == -1){
                printf("Error uploading file.\n");
            }
        }else if(!strcmp(user_cmd, "download")) {
           if (downloadFile(socket,user_arg) == -1){
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
            if (exit_client() == -1){
                printf("Error exiting client.\n");
            }else{
                session_alive = 0;
            }
        }else{
            printf("\nInvalid input!\n");
            print_cli_options();
        }
    } while(session_alive);
}

int main(int argc, char const *argv[]){
    char username[64];
    struct hostent *host;

    if(argc < 3){
        fprintf(stderr, "ERROR! Invalid number of arguments.\n");
        fprintf(stderr, "Command should be in the form: client <username> <server_ip_address>\n");
        exit(0);
    }

    if ((host = gethostbyname((char *)argv[2])) == NULL){
        fprintf(stderr, "ERROR! No such host\n");
        exit(0);
    }
    
	if (create_local_dir() < 0) 
		exit(0);

    strcpy((char *) username, argv[1]);
    server.ip = *(unsigned long *) host->h_addr;
    server.port = PORT;

    if((socketfd = create_udp_socket()) < 0){
        fprintf(stderr,"ERROR opening socket\n");
        exit(0);
    }

    // Conecta com o servidor e atualiza a porta
    server.port = hello(socketfd, server, username);

    printf("📡 Client connected to %s:%d\n\n", inet_ntoa(*(struct in_addr *) &server.ip), server.port);

    run_cli(socketfd);
    
    return 0;
}
