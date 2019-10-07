#include <stdio.h>
#include <stdlib.h>
#include "../include/server.h"
#include "../include/communication.h"
#include "../include/filesystem.h"
#include <utime.h>
#include <semaphore.h> 
#include <search.h>

int listen_socket;
sem_t file_is_created;

/** 
 *  Escuta um cliente em um determinado socket 
 * */
void *thread_client_cmd(void *thread_info){
	char user_dir[MAX_PATH_LENGTH], download_file_path[MAX_PATH_LENGTH];
	THREAD_INFO info = *((THREAD_INFO *) thread_info);
	REMOTE_ADDR addr = info.client.client_addr;
	PACKET msg;
    COMMAND *cmd;
	FILE_INFO file_info;
	int n, socket = info.sock_cmd;

	//Encontra os mutexes associados ao usuário
	ENTRY to_search;
	ENTRY *found;
	to_search.key = info.client.username;
	found = hsearch(to_search,FIND);

	// Path da pasta do usuário no servidor
	strcpy(user_dir, SERVER_DIR);	
	strcat(user_dir, info.client.username);
	strcat(user_dir, "/");

    while(1){
		n = recv_packet(socket, NULL, &msg, 0);

		if (n < 0){
			printf("ERROR recvfrom:  %s\n", strerror(errno));
			pthread_exit(NULL);
		}

        if(msg.header.type == CMD){
            cmd = (COMMAND *) &msg.data;

            switch((*cmd).code){
                case UPLOAD:
					pthread_mutex_lock(&(((CLIENT_MUTEX*)found->data)->sync_or_command));
					file_info = *((FILE_INFO*)cmd->argument);
                    printf("📝 [%s:%d] CMD: UPLOAD %s\n", inet_ntoa(*(struct in_addr *) &addr.ip), addr.port, file_info.filename);
					receive_file(file_info, user_dir, socket);
					pthread_mutex_unlock(&(((CLIENT_MUTEX*)found->data)->sync_or_command));
                   
                    break;
                case DOWNLOAD:
					
                    if(strlen((*cmd).argument) > 0){
						pthread_mutex_lock(&(((CLIENT_MUTEX*)found->data)->sync_or_command));
                        printf("📝 [%s:%d] CMD: DOWNLOAD %s\n", inet_ntoa(*(struct in_addr *) &addr.ip), addr.port, cmd->argument);		
						strcpy(download_file_path, user_dir);
						strcat(download_file_path, cmd->argument);		
						send_file(addr, download_file_path);
						pthread_mutex_unlock(&(((CLIENT_MUTEX*)found->data)->sync_or_command));			
                    }else
                        printf("ERROR: download missing argument\n"); 

                    break;
                case DELETE:
					
                    if(strlen((*cmd).argument) > 0){
						pthread_mutex_lock(&(((CLIENT_MUTEX*)found->data)->sync_or_command));
                        printf("📝 [%s:%d] CMD: DELETE %s\n", inet_ntoa(*(struct in_addr *) &addr.ip), addr.port, cmd->argument);
						delete(cmd->argument, user_dir);
						pthread_mutex_unlock(&(((CLIENT_MUTEX*)found->data)->sync_or_command));
                    }else
                        printf("ERROR: delete missing argument\n");
                    
                    break;
                case LST_SV:
					pthread_mutex_lock(&(((CLIENT_MUTEX*)found->data)->sync_or_command));
                    printf("📝 [%s:%d] CMD: LST_SV\n", inet_ntoa(*(struct in_addr *) &addr.ip), addr.port);
					list_server(socket, user_dir, addr);
					pthread_mutex_unlock(&(((CLIENT_MUTEX*)found->data)->sync_or_command));
                    break;
                case EXIT:
					pthread_mutex_lock(&(((CLIENT_MUTEX*)found->data)->sync_or_command));
                    printf("📝 [%s:%d] BYE: client disconnected\n", inet_ntoa(*(struct in_addr *) &addr.ip), addr.port);
					pthread_mutex_unlock(&(((CLIENT_MUTEX*)found->data)->sync_or_command));
					pthread_exit(NULL);

                    break;
                default:
                    printf("✉ [%s:%d] WARNING: Message ignored by CMD thread. Wrong CMD code.\n", inet_ntoa(*(struct in_addr *) &addr.ip), addr.port);
            }
        }else
			printf("✉ [%s:%d] WARNING: Message ignored by CMD thread. Wrong type.\n", inet_ntoa(*(struct in_addr *) &addr.ip), addr.port);	
    }
}

void *thread_client_sync(void *thread_info){
	char user_dir[MAX_PATH_LENGTH], download_file_path[MAX_PATH_LENGTH];
	THREAD_INFO info = *((THREAD_INFO *) thread_info);
	REMOTE_ADDR addr = info.client.client_addr;
	PACKET msg;
	COMMAND *cmd;
	FILE_INFO file_info;
	struct sockaddr_in cli_addr;
	int n, socket = info.sock_sync;

	//Encontra os mutexes associados ao usuário
	ENTRY to_search;
	ENTRY *found;
	to_search.key = info.client.username;
	found = hsearch(to_search,FIND);

	sem_wait(&file_is_created);
	sem_destroy(&file_is_created);

	cli_addr.sin_family = AF_INET;
    cli_addr.sin_port = htons(addr.port);
    cli_addr.sin_addr.s_addr = addr.ip;
    bzero(&(cli_addr.sin_zero), 8);

	// Path da pasta do usuário no servidor
	strcpy(user_dir, SERVER_DIR);	
	strcat(user_dir, info.client.username);
	strcat(user_dir, "/");

    while(1){
		n = recv_packet(socket, &addr, &msg, 0);

		if (n < 0){
			printf("ERROR recvfrom:  %s\n", strerror(errno));
			pthread_exit(NULL);
		}

		cmd = (COMMAND *) &msg.data;

        if(msg.header.type == CMD){
			switch((*cmd).code){
				case SYNC_DIR:
					pthread_mutex_lock(&(((CLIENT_MUTEX*)found->data)->sync_or_command));
					//printf("📝 [%s:%d] SYNC: SYNC_DIR - Iniciando sincronização de %s\n", inet_ntoa(*(struct in_addr *) &addr.ip), addr.port, info.client.username);
					sync_user(socket, user_dir, addr);
					pthread_mutex_unlock(&(((CLIENT_MUTEX*)found->data)->sync_or_command));
					
					break;
				case UPLOAD:
					file_info = *((FILE_INFO*)cmd->argument);
                    printf("📝 [%s:%d] SYNC: UPLOAD %s\n", inet_ntoa(*(struct in_addr *) &addr.ip), addr.port, file_info.filename);
					receive_file(file_info, user_dir, socket);
                   
                    break;
                case DOWNLOAD:
                    if(strlen((*cmd).argument) > 0){
                        printf("📝 [%s:%d] SYNC: DOWNLOAD %s\n", inet_ntoa(*(struct in_addr *) &addr.ip), addr.port, cmd->argument);		
						strcpy(download_file_path, user_dir);
						strcat(download_file_path, cmd->argument);		
						send_file(addr, download_file_path);			
                    }else
                        printf("ERROR: download missing argument\n"); 

                    break;
                case DELETE:
                    if(strlen((*cmd).argument) > 0){
                        printf("📝 [%s:%d] SYNC: DELETE %s\n", inet_ntoa(*(struct in_addr *) &addr.ip), addr.port, cmd->argument);
						if(delete(cmd->argument, user_dir) <0)
							printf("Error deleting file : %s\n", strerror(errno));
                    }else
                        printf("ERROR: delete missing argument\n");
                    
                    break;
			}
				
			
        }else
			printf("✉ [%s:%d] WARNING: Message ignored by SYNC thread. Wrong type.\n", inet_ntoa(*(struct in_addr *) &addr.ip), addr.port);
    }  
}

int hello(CONNECTION_INFO conn){
	PACKET packet;

	packet.header.type = HELLO;
	memcpy(&packet.data, &conn.ports, sizeof(SERVER_PORTS_FOR_CLIENT));

	return send_packet(listen_socket, conn.client.client_addr, packet, 0);
}

int new_client(CLIENT_INFO *client){
    pthread_t thr_cmd, thr_sync;
	int sock_cmd, sock_sync;
	THREAD_INFO thread_info;
	CONNECTION_INFO conn_info;

    sock_cmd = create_udp_socket();
    if(sock_cmd < 0){
        printf("ERROR creating new socket\n");
        return -1;
    }
	bind_udp_socket(sock_cmd, INADDR_ANY, 0);

	sock_sync = create_udp_socket();
    if(sock_sync < 0){
        printf("ERROR creating new socket\n");
        return -1;
    }
	bind_udp_socket(sock_sync, INADDR_ANY, 0);


	thread_info.client = *client;
	thread_info.sock_cmd = sock_cmd;
	thread_info.sock_sync = sock_sync;

	conn_info.client = *client;
	conn_info.ports.port_cmd = get_socket_port(sock_cmd);
	conn_info.ports.port_sync = get_socket_port(sock_sync);

	pthread_create(&thr_cmd, NULL, thread_client_cmd, &thread_info);
	pthread_create(&thr_sync, NULL, thread_client_sync, &thread_info);

	if(hello(conn_info) < 0){
		printf("ERROR responding HELLO message\n");
		return -1;
	}

    return 0;
}
int sync_user(int socket, char *user_dir, REMOTE_ADDR client_addr){
	DIR_ENTRY *server_entries = malloc(sizeof(DIR_ENTRY));
	int n_server_ent;
	int n_packets, n, packet_number = 0;
	PACKET entries_pkt;

	n_server_ent = get_dir_status(user_dir, &server_entries);
	n_packets = (n_server_ent * sizeof(DIR_ENTRY)) % DATA_LENGTH ? ((n_server_ent * sizeof(DIR_ENTRY)) / DATA_LENGTH) + 1 : ((n_server_ent * sizeof(DIR_ENTRY)) / DATA_LENGTH);

	do{
        entries_pkt.header.type = DATA;
        entries_pkt.header.seqn = packet_number;
        entries_pkt.header.total_size = n_packets;     
        if(packet_number == (n_packets - 1))
            entries_pkt.header.length = (n_server_ent * sizeof(DIR_ENTRY)) % DATA_LENGTH;
        else
            entries_pkt.header.length = DATA_LENGTH;

		memcpy(&entries_pkt.data, ((char*) server_entries) + (packet_number*DATA_LENGTH), entries_pkt.header.length);
		packet_number++;
        n = send_packet(socket, client_addr, entries_pkt, 0);

        if(n < 0){
            printf ("Error request_sync send_packet: %s\n", strerror(errno));
            return -1;
        }
    } while(packet_number < n_packets);

	free(server_entries);

	return 0;
}

int list_server(int socket, char *user_dir, REMOTE_ADDR client_addr){
	DIR_ENTRY *server_entries = malloc(sizeof(DIR_ENTRY));
	int n_server_ent;
	int n_packets, n, packet_number = 0;
	PACKET entries_pkt;

	n_server_ent = get_dir_status(user_dir, &server_entries);
	n_packets = (n_server_ent * sizeof(DIR_ENTRY)) % DATA_LENGTH ? ((n_server_ent * sizeof(DIR_ENTRY)) / DATA_LENGTH) + 1 : ((n_server_ent * sizeof(DIR_ENTRY)) / DATA_LENGTH);

	do{
        entries_pkt.header.type = DATA;
        entries_pkt.header.seqn = packet_number;
        entries_pkt.header.total_size = n_packets;     
        if(packet_number == (n_packets - 1))
            entries_pkt.header.length = (n_server_ent * sizeof(DIR_ENTRY)) % DATA_LENGTH;
        else
            entries_pkt.header.length = DATA_LENGTH;

		memcpy(&entries_pkt.data, ((char*) server_entries) + (packet_number*DATA_LENGTH), entries_pkt.header.length);
		packet_number++;
        n = send_packet(socket, client_addr, entries_pkt, 0);

        if(n < 0){
            printf ("Error list_server send_packet: %s\n", strerror(errno));
            return -1;
        }
    } while(packet_number < n_packets);

	free(server_entries);

	return 0;
}

int delete(char *file_name, char *client_dir_path){
	FILE *temp_file;
	//Inicialização do nome do arquivo temporário
	char file_path[FILE_NAME_SIZE];
	char temp_file_path[MAX_PATH_LENGTH];

	strcpy(temp_file_path, client_dir_path);
	strcat(temp_file_path,"~");
	strcat(temp_file_path,file_name);
	
	strcpy(file_path, client_dir_path);
	strcat(file_path, file_name);

	if(remove(file_path) == 0){	
		//Apenas cria o arquivo vazio
		temp_file = fopen(temp_file_path, "wb");
		fclose(temp_file);
		return SUCCESS;
	} else if(remove(temp_file_path) == 0){
		// Arquivo temporário removido
		return SUCCESS;
	} else{
		printf("\nError deleting file.\n");
		return -1;
	}
}

int main(int argc, char const *argv[]){
	int n;
	struct sockaddr_in cli_addr;
	socklen_t clilen = sizeof(cli_addr);
	PACKET msg;
	REMOTE_ADDR client_addr;
	CLIENT_INFO client_info;

	hcreate(NUM_OF_MAX_CONNECTIONS);


    listen_socket = create_udp_socket();
    listen_socket = bind_udp_socket(listen_socket, INADDR_ANY, PORT);

    if(listen_socket < 0)
        return -1;
 
    memset(&cli_addr, 0, sizeof(cli_addr));
    clilen = sizeof(struct sockaddr_in);

    while (1) {
		n = recvfrom(listen_socket, &msg, PACKET_SIZE, 0, (struct sockaddr *) &cli_addr, &clilen);

		sem_init(&file_is_created, 0, 0);
		
		if (n < 0) 
			printf("ERROR on recvfrom\n");

		client_addr.ip = cli_addr.sin_addr.s_addr;
		client_addr.port = ntohs(cli_addr.sin_port);

		if(msg.header.type == HELLO){
			//Seta informações de client_info. WARNING: Caso seja feito alteração no pacote enviado, deve-se alterar
			//para que  o msg.data.data do memcpy seja só o username.
			client_info.client_addr = client_addr;
			strcpy(client_info.username, (char *) msg.data);

			ENTRY user_to_search;
			ENTRY *user_retrieved;
			user_to_search.key = client_info.username;
			//Caso tenha achado um usuário, incrementa o número de usuários logados
			if ((user_retrieved = hsearch(user_to_search,FIND)) != NULL){
				(((CLIENT_MUTEX*)user_retrieved->data)->clients_connected)++;
			}
			//Caso não haja nenhum usuário
			else{
				//Inicializa a nova estrutura de mutex
				CLIENT_MUTEX new_mutex;
				new_mutex.clients_connected = 1;
				pthread_mutex_init(&(new_mutex.sync_or_command), NULL);
				
				//Inicializa nova entry
				ENTRY user_to_add;
				user_to_add.data = (void *) &new_mutex;
				user_to_add.key = client_info.username;
				//Adiciona no hash
				hsearch(user_to_add, ENTER);
			}

			n = ack(listen_socket, (struct sockaddr *)&cli_addr, clilen);
			if(n < 0){
				printf("ERROR ack at HELLO\n");
				exit(0);
			}

			n = new_client(&client_info);
			if(n < 0){
				printf("ERROR creating client sockets\n");
				exit(0);
			}

			if (create_user_dir((char *) &(msg.data)) < 0) 
				exit(0);
			
			sem_post(&file_is_created);
			
			printf("📡 [%s:%d] HELLO: connected as %s\n", inet_ntoa(*(struct in_addr *) &client_addr.ip), client_addr.port,(char *) &(msg.data));
		} else {
			printf("📡 [%s:%d] WARNING: Non-HELLO message ignored.\n", inet_ntoa(*(struct in_addr *) &client_addr.ip), client_addr.port);
		}				
	}

    return 0;
}
