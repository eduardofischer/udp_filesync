#include <stdio.h>
#include <stdlib.h>
#include "../include/server.h"
#include "../include/communication.h"
#include "../include/filesystem.h"

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
	COMMAND *cmd;
	struct sockaddr_in cli_addr;
	socklen_t clilen = sizeof(cli_addr);
	int n, socket = info.sock_cmd;

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
                    if(strlen((*cmd).argument) > 0){
                        printf("📝 [%s:%d] CMD: UPLOAD - Uploaded %s to %s\n", inet_ntoa(*(struct in_addr *) &addr.ip), addr.port, (*cmd).argument, username);
                        ack(socket, (struct sockaddr *) &cli_addr, clilen);
						upload(cmd->argument, user_dir, socket);
                    }else{
                        err(socket, (struct sockaddr *) &cli_addr, clilen, "UPLOAD missing argument");      
                    }
                    break;
                case DOWNLOAD:
                    if(strlen((*cmd).argument) > 0){
                        printf("📝 [%s:%d] CMD: DOWNLOAD %s\n", inet_ntoa(*(struct in_addr *) &addr.ip), addr.port, (*cmd).argument);
                        ack(socket, (struct sockaddr *) &cli_addr, clilen);
                    }else
                        err(socket, (struct sockaddr *) &cli_addr, clilen, "DOWNLOAD missing argument"); 
                    break;
                case DELETE:
                    if(strlen((*cmd).argument) > 0){
                        printf("📝 [%s:%d] CMD:: DELETE %s\n", inet_ntoa(*(struct in_addr *) &addr.ip), addr.port, (*cmd).argument);
                        ack(socket, (struct sockaddr *) &cli_addr, clilen);
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
	socklen_t clilen = sizeof(cli_addr);
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
		n = recvfrom(socket, &msg, PACKET_SIZE, 0, (struct sockaddr *) &cli_addr, &clilen);

		if (n < 0){
			printf("ERROR recvfrom:  %s\n", strerror(errno));
			pthread_exit(NULL);
		}

		cmd = (COMMAND *) &msg.data;

        if(msg.header.type == CMD && (*cmd).code == SYNC_DIR){
			printf("📝 [%s:%d] CMD: SYNC_DIR - Iniciando sincronização de %s\n", inet_ntoa(*(struct in_addr *) &addr.ip), addr.port, username);

			ack(socket, (struct sockaddr *) &cli_addr, clilen);
			sync_user(socket, user_dir);
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

int upload(char *filename, char *user_dir, int socket){
	FILE *new_file;
	struct sockaddr_in source_addr;
	socklen_t source_addr_len = sizeof(source_addr);
	char file_path[MAX_PATH_LENGTH];
	int last_received_packet, final_packet, n, first_message_not_received = 1;
	PACKET received;

	//Prepare archive path
	strcpy(file_path, user_dir);
	strcat(file_path, filename);
	
	new_file = fopen(file_path, "wb");
	
	if(isOpened(new_file)){
		do{
			n = recvfrom(socket, (void*) &received, PACKET_SIZE, 0, (struct sockaddr *) &source_addr, &source_addr_len);
			if (n < 0){
				fprintf(stderr, "ERROR receiving upload data: %s\n", strerror(errno));
			err(socket, (struct sockaddr *) &source_addr, source_addr_len, "SERVER ERROR receiving upload data");
			}
			else{	//Message correctly received
				ack(socket,(struct sockaddr *) &source_addr,source_addr_len);
				first_message_not_received = 0; //The first message was received
				write_packet_to_the_file(&received,new_file);
			}
			final_packet = received.header.total_size - 1;
			last_received_packet = received.header.seqn;

		}while(first_message_not_received);

	
		while(last_received_packet < final_packet){
			n = recvfrom(socket, (void*) &received, PACKET_SIZE, 0, (struct sockaddr *) &source_addr, &source_addr_len);
			if (n >= 0){
				ack(socket,(struct sockaddr *) &source_addr,source_addr_len);
				write_packet_to_the_file(&received,new_file);
				last_received_packet = received.header.seqn;
			}
			else{
				err(socket, (struct sockaddr *) &source_addr, source_addr_len, "Error receiving the message");
			}
		}

		fclose(new_file);
		return SUCCESS;
	}
	else{
		printf("Erro ao criar arquivo em função de upload, tentou-se criar o arquivo: %s", file_path);
		return ERR_OPEN_FILE;
	}
}

int sync_user(int socket, char *user_dir){
	DIR_ENTRY *server_entries = malloc(sizeof(DIR_ENTRY));
	DIR_ENTRY *client_entries = malloc(sizeof(DIR_ENTRY));
	int n_server_ent, n_client_ent, client_length = 0;
	int n_packets, n, last_recv_packet;
	struct sockaddr_in source_addr;
	socklen_t source_addr_len = sizeof(source_addr);
	PACKET recv;

	n_server_ent = get_dir_status(user_dir, &server_entries);

	do {
		n = recvfrom(socket, (void*) &recv, PACKET_SIZE, 0, (struct sockaddr *) &source_addr, &source_addr_len);
		if (n < 0){
			fprintf(stderr, "ERROR receiving client_entries: %s\n", strerror(errno));
			err(socket, (struct sockaddr *) &source_addr, source_addr_len, "SERVER ERROR receiving client_entries");
		}
		else {	//Message correctly received
			ack(socket,(struct sockaddr *) &source_addr,source_addr_len);
			client_entries = realloc(client_entries, client_length + recv.header.length);
			memcpy(client_entries + client_length, &recv.data, recv.header.length);
			client_length += recv.header.length;
		}

		n_packets = recv.header.total_size;
		last_recv_packet = recv.header.seqn;
	} while(last_recv_packet < n_packets - 1);

	n_client_ent = client_length / sizeof(DIR_ENTRY);

	printf("Entradas no servidor:\n");
	print_dir_status(&server_entries, n_server_ent);

	printf("Entradas no Cliente:\n");
	print_dir_status(&client_entries, n_client_ent);

	free(server_entries);
	free(client_entries);

	return 0;
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
