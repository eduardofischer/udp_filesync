#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "../include/client.h"
#include "../include/communication.h"
#include "../include/filesystem.h"
    
int sock_cmd, sock_sync;
char username[64];
REMOTE_ADDR server_cmd;
REMOTE_ADDR server_sync;

/** Envia o arquivo para o servidor **/
int uploadFile(char* filePath){
    FILE *file_to_upload;

    //Para testar se o arquivo existe
    file_to_upload = fopen(filePath,"rb");

    //Caso exista, fecha e chama send_file
    if(file_to_upload != NULL){
        fclose(file_to_upload);
        return send_file(server_cmd,filePath);
    }

    //Caso nao exista retorna erro
     printf("Ψ༼ຈل͜ຈ༽Ψ Satan says that the file doesn't exist.  (Typo?) Ψ༼ຈل͜ຈ༽Ψ");
    return -1;
}

/** Faz o download de um arquivo do servidor **/
int downloadFile(int socket, char *filename){
    int n;
    PACKET msg;
    struct sockaddr_in cli_addr;
	socklen_t clilen = sizeof(cli_addr);
    FILE_INFO file_info;
    COMMAND *cmd;

    //Envia um comando de download
    send_command(socket, server_cmd, DOWNLOAD, filename);
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
        response = send_command(socketDataTransfer, server_cmd, DELETE, fileName);
        
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
    DIR_ENTRY *server_entries = malloc(sizeof(DIR_ENTRY));
    int n_packets, n, last_recv_packet, server_length = 0;
    int n_server_ent, i;
    PACKET recv_entries_pkt;

    if(send_command(sock_cmd, server_cmd, LST_SV, NULL) < 0){
        printf("ERROR sending list_server cmd: %s\n", strerror(errno));
        return -1;
    }

    do {
        n = recv_packet(sock_cmd, NULL, &recv_entries_pkt);
		if (n < 0){
			fprintf(stderr, "ERROR receiving server_entries: %s\n", strerror(errno));
		} else {	//Message correctly received
			server_entries = realloc(server_entries, server_length + recv_entries_pkt.header.length + 1);
			memcpy((char*)server_entries + server_length, recv_entries_pkt.data, recv_entries_pkt.header.length);
            server_length += recv_entries_pkt.header.length;
		}
		n_packets = recv_entries_pkt.header.total_size;
		last_recv_packet = recv_entries_pkt.header.seqn;
	} while(last_recv_packet < n_packets - 1);

    n_server_ent = server_length / sizeof(DIR_ENTRY);

    for(i=0; i < n_server_ent; i++){
        printf("%s\n", server_entries[i].name);
    }

    return 0;
};

/** Lista os arquivos salvos no diretório “sync_dir” **/
int list_client(){
    DIR_ENTRY *entries = malloc(sizeof(DIR_ENTRY));
    int n_entries;

    n_entries = get_dir_status(LOCAL_DIR, &entries);
    print_dir_status(&entries, n_entries);

    free(entries);

    return n_entries;
};

/** Cria o diretório “sync_dir” e inicia as atividades de sincronização **/
int getSyncDir(){
    return -1;
};

/** Fecha a sessão com o servidor **/
int exit_client(){
    return send_command(sock_cmd, server_cmd, EXIT, NULL);
};

/** 
 *  Inicia a comunicação de um cliente com o servidor 
 *  Retorna a porta com a qual o cliente deve se comunicar
 *  ou -1 em caso de erro
*/
int hello(char *username){
    PACKET packet, response;
    int n;

    packet.header.type = HELLO;
    strcpy((char *)&(packet.data), username);

    n = send_packet(sock_cmd, server_cmd, packet);

    if (n < 0){
        fprintf(stderr, "ERROR! HELLO failed\n");
        return -1;;
    }

    if(recv_packet(sock_cmd, NULL, &response) < 0){
        printf("ERROR recv_packet\n");
        return -1;
    }
    
    if(response.header.type == HELLO){
        server_cmd.port = ((SERVER_PORTS_FOR_CLIENT *)&response.data)->port_cmd;
        server_sync.port = ((SERVER_PORTS_FOR_CLIENT *)&response.data)->port_sync;
    }

    return 0;
}

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
            if (list_client() == -1){
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

int request_sync(){
    DIR_ENTRY *entries = malloc(sizeof(DIR_ENTRY));
    DIR_ENTRY *server_entries = malloc(sizeof(DIR_ENTRY));
    int n_entries, n_packets, n, last_recv_packet, server_length = 0;
    int n_server_ent, i;
    PACKET recv_entries_pkt;
    SYNC_LIST list;
    // FILE_INFO file_info;
    // struct stat file_stat;
    // char file_path[MAX_PATH_LENGTH];

    n_entries = get_dir_status(LOCAL_DIR, &entries);

    if(send_command(sock_sync, server_sync, SYNC_DIR, NULL) < 0){
        printf("ERROR sending sync cmd: %s\n", strerror(errno));
        return -1;
    }

    do {
        n = recv_packet(sock_sync, NULL, &recv_entries_pkt);
		if (n < 0){
			fprintf(stderr, "ERROR receiving server_entries: %s\n", strerror(errno));
		} else {	//Message correctly received
			server_entries = realloc(server_entries, server_length + recv_entries_pkt.header.length + 1);
			memcpy((char*)server_entries + server_length, recv_entries_pkt.data, recv_entries_pkt.header.length);
            server_length += recv_entries_pkt.header.length;
		}
		n_packets = recv_entries_pkt.header.total_size;
		last_recv_packet = recv_entries_pkt.header.seqn;
	} while(last_recv_packet < n_packets - 1);

	n_server_ent = server_length / sizeof(DIR_ENTRY);

    compare_entry_diff(server_entries, entries, n_server_ent, n_entries, &list);

    //printf("Server entries:\n");
    //print_dir_status(&server_entries, n_server_ent);

    //printf("Client entries:\n");
    //print_dir_status(&entries, n_entries);

    printf("\n\nLista de DOWNLOAD:\n");
	for(i=0; i<list.n_downloads; i++){
        printf("- %s\n", (char *)(list.list + i * MAX_NAME_LENGTH));
        //downloadFile(sock_sync, (char *)(list.list + i * MAX_NAME_LENGTH));
    }
		
	printf("Lista de UPLOAD:\n");
	for(i=0; i<list.n_uploads; i++){
        printf("- %s\n", (char *)(list.list + (list.n_downloads + i) * MAX_NAME_LENGTH));
    }

	printf("\n");

    free(entries);
    free(server_entries);
    return 0;
}

void *sync_files(){
    if(request_sync() < 0)
        printf ("Error sync_files request_sync: %s\n", strerror(errno));

    while(1){
        // use inotify to request_sync when a file changes
    }
}

int main(int argc, char const *argv[]){
    struct hostent *host;
    pthread_t sync_thread;

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
    server_cmd.ip = *(unsigned long *) host->h_addr;
    server_sync.ip = *(unsigned long *) host->h_addr;
    server_cmd.port = PORT;

    if((sock_cmd = create_udp_socket()) < 0){
        fprintf(stderr,"ERROR opening socket\n");
        exit(0);
    }

    if((sock_sync = create_udp_socket()) < 0){
        fprintf(stderr,"ERROR opening socket\n");
        exit(0);
    }

    // Conecta com o servidor e atualiza as portas
    if(hello(username) < 0)
        fprintf(stderr,"ERROR connecting to server\n");

    printf("📡 Client connected to %s:%d\n\n", inet_ntoa(*(struct in_addr *) &server_cmd.ip), server_cmd.port);

    pthread_create(&sync_thread, NULL, sync_files, NULL);

    run_cli(sock_cmd);
    
    return 0;
}
