#include <stdio.h>
#include <stdlib.h>
#include "../include/server.h"
#include "../include/communication.h"
#include "../include/filesystem.h"
#include <utime.h>

int listen_socket;




/** 
 *  Escuta um cliente em um determinado socket 
 * */
void *thread_client_cmd(void *thread_info){
	char username[MAX_NAME_LENGTH];
	char user_dir[MAX_PATH_LENGTH];
	THREAD_INFO info = *((THREAD_INFO *) thread_info);
	REMOTE_ADDR addr = info.client.client_addr;
	PACKET msg;
	struct sockaddr_in cli_addr;
	socklen_t clilen = sizeof(cli_addr);
	int n, socket = info.sock_cmd;
    COMMAND *cmd;
	FILE_INFO file_info;
	struct stat stats;
	char storage_root[15] = "user_data/";   
	char storage_client[MAX_PATH_LENGTH];
	char download_file_path[MAX_PATH_LENGTH];

	//Seta o path para o armazenamento de dados do cliente que solicitou algo ao servidor
	//Consiste de uma pasta raiz para todos os clientes, mais pastas para cada cliente.
	strcpy(storage_client,storage_root);
	strcat(storage_client,(info.client).username);

	printf("%s", storage_client);

	cli_addr.sin_family = AF_INET;
    cli_addr.sin_port = htons(addr.port);
    cli_addr.sin_addr.s_addr = addr.ip;
    bzero(&(cli_addr.sin_zero), 8);

	strcpy(username, info.client.username);

	// Path da pasta do usuário no servidor
	strcpy(user_dir, SERVER_DIR);	
	strcat(user_dir, username);
	strcat(user_dir, "/");

    while(1){
		n = recvfrom(socket, &msg, PACKET_SIZE, 0, (struct sockaddr *) &cli_addr, &clilen);

		if (n < 0){
			printf("ERROR recvfrom:  %s\n", strerror(errno));
			pthread_exit(NULL);
		}

        if(msg.header.type == CMD){
            cmd = (COMMAND *) &msg.data;

            switch((*cmd).code){
                case UPLOAD:
					file_info = *((FILE_INFO*)cmd->argument);
                    printf("📝 [%s:%d] CMD: UPLOAD %s\n", inet_ntoa(*(struct in_addr *) &addr.ip), addr.port, (*cmd).argument);
                    ack(socket, (struct sockaddr *) &cli_addr, clilen);
					receive_file(file_info,storage_client,socket);
                   
                    break;
                case DOWNLOAD:
                    if(strlen((*cmd).argument) > 0){
                        printf("📝 [%s:%d] CMD: DOWNLOAD %s\n", inet_ntoa(*(struct in_addr *) &addr.ip), addr.port, (*cmd).argument);
                        ack(socket, (struct sockaddr *) &cli_addr, clilen);
						
						strcpy(download_file_path,storage_client);
						strcat(download_file_path,"/");
						strcat(download_file_path,cmd->argument);
						stat(download_file_path,&stats);
						
						strcpy(file_info.filename,cmd->argument);
						file_info.access_time = stats.st_atime;
						file_info.modification_time = stats.st_mtime;
						printf("%s", download_file_path);
						send_file(addr,download_file_path);
						
                    }else
                        err(socket, (struct sockaddr *) &cli_addr, clilen, "DOWNLOAD missing argument"); 
                    break;
                case DELETE:
                    if(strlen((*cmd).argument) > 0){
                        printf("📝 [%s:%d] CMD:: DELETE %s\n", inet_ntoa(*(struct in_addr *) &addr.ip), addr.port, (*cmd).argument);
                        ack(socket, (struct sockaddr *) &cli_addr, clilen);
						char file_name[FILE_NAME_SIZE];
						strcpy(file_name, cmd->argument); 
						delete(file_name, storage_client);
						break;
                    }else
                        err(socket, (struct sockaddr *) &cli_addr, clilen, "DELETE missing argument");
                    
                    break;
                case LST_SV:
                    printf("📝 [%s:%d] CMD: LST_SV\n", inet_ntoa(*(struct in_addr *) &addr.ip), addr.port);
                    ack(socket, (struct sockaddr *) &cli_addr, clilen);
                    break;
                case EXIT:
                    printf("📝 [%s:%d] CMD: EXIT\n", inet_ntoa(*(struct in_addr *) &addr.ip), addr.port);
                    ack(socket, (struct sockaddr *) &cli_addr, clilen);
					pthread_exit(NULL);
                    break;
                default:
                    fprintf(stderr, "❌ ERROR Invalid Command\n");
                    err(socket, (struct sockaddr *) &cli_addr, clilen, "Invalid command");
            }
        }else
			printf("✉ [%s:%d] WARNING: Message ignored by CMD thread. Wrong type.\n", inet_ntoa(*(struct in_addr *) &addr.ip), addr.port);
		
    }
}

void *thread_client_sync(void *thread_info){
	char username[MAX_NAME_LENGTH];
	char user_dir[MAX_PATH_LENGTH];
	THREAD_INFO info = *((THREAD_INFO *) thread_info);
	REMOTE_ADDR addr = info.client.client_addr;
	PACKET msg;
	COMMAND *cmd;
	struct sockaddr_in cli_addr;
	int n, socket = info.sock_sync;

	cli_addr.sin_family = AF_INET;
    cli_addr.sin_port = htons(addr.port);
    cli_addr.sin_addr.s_addr = addr.ip;
    bzero(&(cli_addr.sin_zero), 8);

	strcpy(username, info.client.username);

	// Path da pasta do usuário no servidor
	strcpy(user_dir, SERVER_DIR);	
	strcat(user_dir, username);
	strcat(user_dir, "/");

    while(1){
		n = recv_packet(socket, &addr, &msg);

		if (n < 0){
			printf("ERROR recvfrom:  %s\n", strerror(errno));
			pthread_exit(NULL);
		}

		cmd = (COMMAND *) &msg.data;

        if(msg.header.type == CMD && (*cmd).code == SYNC_DIR){
			printf("📝 [%s:%d] CMD: SYNC_DIR - Iniciando sincronização de %s\n", inet_ntoa(*(struct in_addr *) &addr.ip), addr.port, username);
			sync_user(socket, user_dir, addr);
        }else
			printf("✉ [%s:%d] WARNING: Message ignored by SYNC thread. Wrong type.\n", inet_ntoa(*(struct in_addr *) &addr.ip), addr.port);
    }  
}

int hello(CONNECTION_INFO conn){
	PACKET packet;

	packet.header.type = HELLO;
	memcpy(&packet.data, &conn.ports, sizeof(SERVER_PORTS_FOR_CLIENT));

	return send_packet(listen_socket, conn.client.client_addr, packet);
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
	n_packets = ceil((n_server_ent * sizeof(DIR_ENTRY)) / (double) DATA_LENGTH);

	while(packet_number < n_packets){
        entries_pkt.header.type = DATA;
        entries_pkt.header.seqn = packet_number;
        entries_pkt.header.total_size = n_packets;     
        if(packet_number == n_packets - 1)
            entries_pkt.header.length = (n_server_ent * sizeof(DIR_ENTRY)) % DATA_LENGTH;
        else
            entries_pkt.header.length = DATA_LENGTH;

        memcpy(&entries_pkt.data, server_entries + packet_number*sizeof(DIR_ENTRY), entries_pkt.header.length);

        n = send_packet(socket, client_addr, entries_pkt);
        if(n < 0){
            printf ("Error request_sync send_packet: %s\n", strerror(errno));
            return -1;
        }
    }
	
	free(server_entries);

	return 0;
}

int delete(char *file_name, char *client_folder_path){
	char private_path_copy[FILE_NAME_SIZE];
	strcpy(private_path_copy, client_folder_path);
	strcat(private_path_copy, "/");
	char *target = strcat(private_path_copy, file_name);
	if(remove(target) == 0){
		return SUCCESS;
	}
	else{
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

    listen_socket = create_udp_socket();
    listen_socket = bind_udp_socket(listen_socket, INADDR_ANY, PORT);

    if(listen_socket < 0)
        return -1;
 
    memset(&cli_addr, 0, sizeof(cli_addr));
    clilen = sizeof(struct sockaddr_in);

    while (1) {
		n = recvfrom(listen_socket, &msg, PACKET_SIZE, 0, (struct sockaddr *) &cli_addr, &clilen);
		
		if (n < 0) 
			printf("ERROR on recvfrom\n");

		client_addr.ip = cli_addr.sin_addr.s_addr;
		client_addr.port = ntohs(cli_addr.sin_port);

		if(msg.header.type == HELLO){
			//Seta informações de client_info. WARNING: Caso seja feito alteração no pacote enviado, deve-se alterar
			//para que  o msg.data.data do memcpy seja só o username.
			client_info.client_addr = client_addr;
			strcpy(client_info.username, (char *) msg.data);

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
			
			printf("📡 [%s:%d] HELLO: connected as %s\n", inet_ntoa(*(struct in_addr *) &client_addr.ip), client_addr.port,(char *) &(msg.data));
		} else {
			printf("📡 [%s:%d] WARNING: Non-HELLO message ignored.\n", inet_ntoa(*(struct in_addr *) &client_addr.ip), client_addr.port);
		}				
	}

    return 0;
}
