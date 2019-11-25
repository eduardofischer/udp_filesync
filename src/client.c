#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "../include/client.h"
#include "../include/communication.h"
#include "../include/filesystem.h"

int sock_cmd, sock_sync;
char username[64];
REMOTE_ADDR server_cmd;
REMOTE_ADDR server_sync;

/** Envia o arquivo para o servidor **/
int uploadFile(char* filePath, REMOTE_ADDR remote){
    FILE *file_to_upload;

    //Para testar se o arquivo existe
    file_to_upload = fopen(filePath,"rb");

    //Caso exista, fecha e chama send_file
    if(file_to_upload != NULL){
        fclose(file_to_upload);
        return send_file(remote, filePath);
    }

    //Caso nao exista retorna erro
     printf("File not found :(\n");
    return -1;
}

/** Faz o download de um arquivo do servidor **/
int downloadFile(int socket, char *filename, char *dir_path, REMOTE_ADDR remote){
    int n;
    PACKET msg;
    FILE_INFO file_info;
    COMMAND *cmd;

    //Envia um comando de download
    send_command(socket, remote, DOWNLOAD, filename, 0);

    //Espera pelas mensagens contendo as informações do arquivo (tempos)
    do{
        n = recv_packet(socket, NULL, &msg, 0);
    } while(n < 0);
    //Após recebida a mensagem acessa as informações do arquivo
    cmd = (COMMAND *) &msg.data;
    file_info = *((FILE_INFO*)cmd->argument);

    //Recebe o arquivo informado em file_info.
    return receive_file(file_info, dir_path, socket);
}

/* Exclui um arquivo da pasta sync_dir do usuario */
int delete_file_local(char* file_name){
    char target[FILE_NAME_SIZE];
    strcpy(target, LOCAL_DIR);
    strcat(target, file_name);
	if(remove(target) == 0){
		return SUCCESS;
	}
	else{
		return -1;
	}
};

/** Lista os arquivos salvos no servidor associados ao usuário **/
int listServer(){
    DIR_ENTRY *server_entries = malloc(sizeof(DIR_ENTRY));
    int n_packets, n, last_recv_packet, server_length = 0;
    int n_server_ent;
    PACKET recv_entries_pkt;

    if(send_command(sock_cmd, server_cmd, LST_SV, NULL, 0) < 0){
        printf("ERROR sending list_server cmd: %s\n", strerror(errno));
        return -1;
    }

    do {
        n = recv_packet(sock_cmd, NULL, &recv_entries_pkt, 0);

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

    if((strlen(server_entries[0].name)) > 0){
        printf("\n## PRINTING SERVER DIRECTORY ##");
        print_dir_status(&server_entries, n_server_ent);
    }
    else
        printf("Server directory is empty.\n");

    free(server_entries);

    return 0;
};

/** Lista os arquivos salvos no diretório “sync_dir” **/
int list_client(){
    DIR_ENTRY *entries = malloc(sizeof(DIR_ENTRY));
    int n_entries;

    n_entries = get_dir_status(LOCAL_DIR, &entries);
    
    if(n_entries > 0){
        printf("\n## PRINTING CLIENT DIRECTORY ##");
        print_dir_status(&entries, n_entries);
    }
    else{
        printf("Client sync directory is empty.\n");
    }

    free(entries);

    return n_entries;
};

/** Cria o diretório “sync_dir” e inicia as atividades de sincronização **/
int getSyncDir(){
    if(create_local_dir() < 0)
        return -1;
    else
        return SUCCESS;    
};

/** Fecha a sessão com o servidor **/
int exit_client(){
    return send_command(sock_cmd, server_cmd, EXIT, NULL, DEFAULT_TIMEOUT);
};



void print_cli_options(){
    printf("\nAvailable commands:\n\n");
    printf("\t📤  upload <path/filename.ext>\n");
    printf("\t📥  download <filename.ext>\n");
    printf("\t❌  delete <filename.ext>\n");
    printf("\t📃  list_server\n");
    printf("\t📃  list_client\n");
    printf("\t🏃  exit\n");
}

// Next three definitions are necessary for command completion.
char *cmds[] = {
    "upload",
    "download",
    "delete",
    "list_server",
    "list_client",
    "exit",
    NULL
};

char *cmd_generator(const char *text, int state){
    static int list_index, len;
    char *cmd;

    if(!state){
        list_index = 0;
        len = strlen(text);
    }

    while ((cmd = cmds[list_index++])){
        if (strncmp(cmd, text, len) == 0){
            return strdup(cmd);
        }
    }

    return NULL;
}

char ** cmd_completion(const char *text, int start, int end){
    return rl_completion_matches(text, cmd_generator);
};

void run_cli(int socket){
    print_cli_options();

    char *user_input;
    char user_input_processing_copy[COMMAND_SIZE];
    char *user_cmd;
    char *user_arg;

    int session_alive = 1;
    do{
        user_input = readline("\nudp_filesync > ");

        // Erases readline feedback from console.
        printf("\033[A\33[2K");
        printf("\033[A\33[2K");

        strcpy(user_input_processing_copy, user_input);
        user_cmd = strtok(user_input_processing_copy, " ");
        user_arg = strtok(NULL, " \n");

        if(user_arg != NULL && strlen(user_arg) > FILE_NAME_SIZE){
            printf("Filename too long, max is 255 characters. Please enter your command again.");
            run_cli(socket);
            exit(0);
        }

        if(user_input != NULL){
            add_history(user_input);
        }
        
        if (!strcmp(user_cmd,"upload")) {
            if (uploadFile(user_arg, server_cmd) == -1){
                printf("Error uploading file.\n");
            }
        }else if(!strcmp(user_cmd, "download")) {
           if (downloadFile(socket, user_arg, "./", server_cmd) == -1){
                printf("Error downloading file.\n");
            }
        }else if(!strcmp(user_cmd, "delete")) {
            if (delete_file_local(user_arg) == -1){
                printf("Error deleting file.\n");
            }
        }else if(!strcmp(user_cmd, "list_server")){
            if (listServer() == -1){
                printf("Error listing server files.\n");
            }
        }else if(!strcmp(user_cmd, "list_client")){
            if (list_client() == -1){
                printf("Error listing client files.\n");
            }
        }else if(!strcmp(user_input, "exit")){
            if (exit_client() == -1){
                printf("Error exiting client.\n");
            }else{
                session_alive = 0;
            }
        }else{
            printf("\nInvalid input!\n");
            print_cli_options();
        }

        free(user_input);
       
    } while(session_alive);
}

int request_sync(){
    DIR_ENTRY *entries = malloc(sizeof(DIR_ENTRY));
    DIR_ENTRY *server_entries = malloc(sizeof(DIR_ENTRY));
    int n_entries, n_packets, last_recv_packet, server_length = 0;
    int n_server_ent;
    PACKET recv_entries_pkt;
    SYNC_LIST list;
    char file_path[MAX_PATH_LENGTH];

    n_entries = get_dir_status(LOCAL_DIR, &entries);

    if(send_command(sock_sync, server_sync, SYNC_DIR, NULL, DEFAULT_TIMEOUT) < 0)
        return -1;

    do {
		if (recv_packet(sock_sync, NULL, &recv_entries_pkt, DEFAULT_TIMEOUT) < 0){
			//fprintf(stderr, "ERROR receiving server_entries: %s\n", strerror(errno));
            return -1;
		} else {	//Message correctly received
            if(recv_entries_pkt.header.total_size > 0){
                server_entries = realloc(server_entries, server_length + recv_entries_pkt.header.length + 1);
                memcpy((char*)server_entries + server_length, recv_entries_pkt.data, recv_entries_pkt.header.length);
                server_length += recv_entries_pkt.header.length;
            }else{
                server_length = 0;
                break;
            }
		}
		n_packets = recv_entries_pkt.header.total_size;
		last_recv_packet = recv_entries_pkt.header.seqn;
	} while(last_recv_packet < n_packets - 1);

	n_server_ent = server_length / sizeof(DIR_ENTRY);

    compare_entry_diff(server_entries, entries, n_server_ent, n_entries, &list);

    if(list.n_downloads > 0){
        strcpy(file_path, LOCAL_DIR);
        downloadFile(sock_sync, (char *)(list.list + 0 * MAX_NAME_LENGTH), file_path, server_sync);
    } else if(list.n_uploads > 0){
        strcpy(file_path, LOCAL_DIR);
        strcat(file_path, (char *)(list.list + (list.n_downloads + 0) * MAX_NAME_LENGTH));
        uploadFile(file_path, server_sync);
    } else if(list.n_deletes > 0){
        delete_file_local((char *)(list.list + (list.n_downloads + list.n_uploads + 0) * MAX_NAME_LENGTH));
    }

    free(list.list);
    free(entries);
    free(server_entries);
    return 0;
}

void *sync_files(){
    int fd, wd, len, i = 0;
    int buf_len = (sizeof(struct inotify_event) + 16) * MAX_N_OF_FILES;
    char buf[buf_len];
    fd_set rfds;
    struct timeval tv;

    tv.tv_sec = 0;
    tv.tv_usec = 0;

    fd = inotify_init();
    if(fd < 0)
        printf ("Error creating inotify fd: %s\n", strerror(errno));

    wd = inotify_add_watch(fd, "sync_dir", IN_DELETE | IN_MOVED_FROM);
    if(wd < 0)
        printf ("Error creating inotify watch: %s\n", strerror(errno));

    while(1){
        i = 0;
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);

        sleep(1);

        if(select(fd + 1, &rfds, NULL, NULL, &tv) > 0 && FD_ISSET(fd, &rfds)){
            len = read(fd, buf, buf_len);
            while (i < len) {
                struct inotify_event *event;
                event = (struct inotify_event *) &buf[i];
                deleteFile(event->name, server_sync);
                i += sizeof (struct inotify_event) + event->len;
            }
        }
        
        request_sync();
    }
}

// Trata o envento CTRL + C
void interruption_handler(int sig){
    signal(sig, SIG_IGN);
    exit_client();
    exit(0);
}

void *front_end(){
    PACKET msg;
    int front_end_socket;

    front_end_socket = create_udp_socket();
    front_end_socket = bind_udp_socket(front_end_socket, INADDR_ANY, FRONT_END_PORT);

    while (1){
        REMOTE_ADDR new_server_addr;
        //Recebe uma mensagem qualquer do servidor indicando que há um novo server principal.
        recv_packet(front_end_socket, &new_server_addr, &msg, 0);

        new_server_addr.port = PORT;
        server_cmd = new_server_addr;
        server_sync = new_server_addr;
        hello(username, front_end_socket, new_server_addr, &server_cmd, &server_sync);
    }
}

int main(int argc, char const *argv[]){
    struct hostent *host;
    pthread_t sync_thread, front_end_thread;

    if(argc < 3){
        fprintf(stderr, "ERROR! Invalid number of arguments.\n");
        fprintf(stderr, "Usage: client <username> <server_ip_address> [<port>]\n");
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

    if(argc > 3)
        server_cmd.port = atoi(argv[3]);
    else
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
    if(hello(username,sock_cmd,server_cmd,&server_cmd,&server_sync) < 0)
        fprintf(stderr,"ERROR connecting to server\n");

    // Captura o evento CTRL + C
    signal(SIGINT, interruption_handler);

    printf("📡 Client connected to %s:%d\n", inet_ntoa(*(struct in_addr *) &server_cmd.ip), server_cmd.port);

    pthread_create(&sync_thread, NULL, sync_files, NULL);
    pthread_create(&front_end_thread, NULL, front_end, NULL);

    rl_attempted_completion_function = cmd_completion;
    run_cli(sock_cmd);
    
    return 0;
}