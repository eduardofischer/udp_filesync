#include <stdio.h>
#include <stdlib.h>
#include "../include/server.h"
#include "../include/communication.h"
#include "../include/filesystem.h"
#include <utime.h>
#include <semaphore.h> 
#include <search.h>
#include <unistd.h>


int listen_socket, port = PORT, inform_device = 3034;
int backup_mode = 0;
int backup_index = -1, backup_socket, n_backup_servers = 0, electing = 0, n_devices = 0;
int alive_delay = ALIVE_DELAY;
sem_t file_is_created;
char hostname[MAX_NAME_LENGTH];
REMOTE_ADDR main_server; // Servidor principal
REMOTE_ADDR alive_addr; 
REMOTE_ADDR *backup_servers; // Lista de servidores de backup~
REMOTE_ADDR *connected_devices; //Lista de devices (USADO SOMENTE EM SERVIDORES DE BACKUP)
sem_t sem_election;
pthread_t thr_backup, thr_alive;

/** 
 *  Escuta um cliente em um determinado socket 
 * */
void *thread_client_cmd(void *thread_info){
	char user_dir[MAX_PATH_LENGTH], download_file_path[MAX_PATH_LENGTH];
	THREAD_INFO info = *((THREAD_INFO *) thread_info);
	REMOTE_ADDR addr;
	PACKET msg;
    COMMAND *cmd;
	FILE_INFO file_info;
	int socket = info.sock_cmd;
	int i;

	//Encontra os mutexes associados ao usuário
	ENTRY to_search;
	ENTRY *found;
	to_search.key = info.client.username;
	found = hsearch(to_search, FIND);
	if(found == NULL)
		printf("Error hsearch FIND: %s\n", strerror(errno));

	// Path da pasta do usuário no servidor
	strcpy(user_dir, SERVER_DIR);	
	strcat(user_dir, info.client.username);
	strcat(user_dir, "/");

    while(1){
		if (recv_packet(socket, &addr, &msg, 0) < 0){
			printf("ERROR recvfrom 1:  %s\n", strerror(errno));
			pthread_exit(NULL);
		}

        if(msg.header.type == CMD){
            cmd = (COMMAND *) &msg.data;

            switch((*cmd).code){
                case UPLOAD:
					pthread_mutex_lock(&((((CLIENT_MUTEX_AND_BACKUP *)(found->data))->client_mutex).sync_or_command));
					file_info = *((FILE_INFO*)cmd->argument);
                    printf("📝 [%s:%d] %s: CMD uploading %s...		", inet_ntoa(*(struct in_addr *) &addr.ip), addr.port, info.client.username, file_info.filename);
					fflush(stdout);
					receive_file(file_info, user_dir, socket);
					
					REMOTE_ADDR *backup_cmd_servers;
					backup_cmd_servers =((CLIENT_MUTEX_AND_BACKUP*)(found->data))->backup_addresses;
					char file_path[MAX_PATH_LENGTH];
					strcpy(file_path,user_dir);
					strcat(file_path,file_info.filename);
					//Envia arquivo recebido para todos os servidores de backup
					for(i = 0; i < n_backup_servers; i++){
						send_file(backup_cmd_servers[i], file_path);
					}
					
					printf("✅ OK!\n");
					pthread_mutex_unlock(&((((CLIENT_MUTEX_AND_BACKUP *)(found->data))->client_mutex).sync_or_command));
                   
                    break;
                case DOWNLOAD:
                    if(strlen((*cmd).argument) > 0){
						pthread_mutex_lock(&((((CLIENT_MUTEX_AND_BACKUP *)(found->data))->client_mutex).sync_or_command));
                        printf("📝 [%s:%d] %s: CMD downloading %s...	", inet_ntoa(*(struct in_addr *) &addr.ip), addr.port, info.client.username, cmd->argument);		
						fflush(stdout);
						strcpy(download_file_path, user_dir);
						strcat(download_file_path, cmd->argument);		
						send_file(addr, download_file_path);
						printf("✅ OK!\n");
						pthread_mutex_unlock(&((((CLIENT_MUTEX_AND_BACKUP *)(found->data))->client_mutex).sync_or_command));			
                    }else
                        printf("ERROR: download missing argument\n"); 

                    break;
                case DELETE:
                    if(strlen((*cmd).argument) > 0){
						pthread_mutex_lock(&((((CLIENT_MUTEX_AND_BACKUP *)(found->data))->client_mutex).sync_or_command));
                        printf("📝 [%s:%d] %s: CMD deleting %s...		", inet_ntoa(*(struct in_addr *) &addr.ip), addr.port, info.client.username, cmd->argument);
						fflush(stdout);
						delete(cmd->argument, user_dir);
						
						REMOTE_ADDR *backup_cmd_servers;
						backup_cmd_servers =((CLIENT_MUTEX_AND_BACKUP*)(found->data))->backup_addresses;
						for(i = 0; i < n_backup_servers; i++){
							deleteFile(cmd->argument,backup_cmd_servers[i]);
						}
						
						printf("✅ OK!\n");
						pthread_mutex_unlock(&((((CLIENT_MUTEX_AND_BACKUP *)(found->data))->client_mutex).sync_or_command));
                    }else
                        printf("ERROR: delete missing argument\n");
                    
                    break;
                case LST_SV:
					pthread_mutex_lock(&((((CLIENT_MUTEX_AND_BACKUP *)(found->data))->client_mutex).sync_or_command));
                    printf("📝 [%s:%d] %s: CMD list_server\n", inet_ntoa(*(struct in_addr *) &addr.ip), addr.port, info.client.username);
					list_server(socket, user_dir, addr);
					pthread_mutex_unlock(&((((CLIENT_MUTEX_AND_BACKUP *)(found->data))->client_mutex).sync_or_command));
                    break;
                case EXIT:
					pthread_mutex_lock(&((((CLIENT_MUTEX_AND_BACKUP *)(found->data))->client_mutex).sync_or_command));
                    printf("📝 [%s:%d] %s: Client disconnected\n", inet_ntoa(*(struct in_addr *) &addr.ip), addr.port, info.client.username);
					pthread_mutex_unlock(&((((CLIENT_MUTEX_AND_BACKUP *)(found->data))->client_mutex).sync_or_command));
					//Envia o comando de deletar o device para os servidores de backup.
					for(i = 0; i < n_backup_servers; i++){
						send_delete_device(socket,backup_servers[i],&addr);
					}
					
					pthread_cancel(info.tid_sync);
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
	REMOTE_ADDR addr;
	PACKET msg;
	COMMAND *cmd;
	FILE_INFO file_info;
	int n, socket = info.sock_sync;
	int i;

	//Encontra os mutexes associados ao usuário
	ENTRY to_search;
	ENTRY *found;
	to_search.key = info.client.username;
	found = hsearch(to_search, FIND);
	if(found == NULL)
		printf("Error hsearch FIND: %s", strerror(errno));

	sem_wait(&file_is_created);
	sem_destroy(&file_is_created);

	// Path da pasta do usuário no servidor
	strcpy(user_dir, SERVER_DIR);	
	strcat(user_dir, info.client.username);
	strcat(user_dir, "/");

    while(1){
		n = recv_packet(socket, &addr, &msg, 0);

		if (n < 0){
			printf("ERROR recvfrom 2:  %s\n", strerror(errno));
			pthread_exit(NULL);
		}

		cmd = (COMMAND *) &msg.data;

        if(msg.header.type == CMD){
			switch((*cmd).code){
				case SYNC_DIR:
					pthread_mutex_lock(&((((CLIENT_MUTEX_AND_BACKUP *)(found->data))->client_mutex).sync_or_command));
					//printf("📝 [%s:%d] SYNC: SYNC_DIR - Iniciando sincronização de %s\n", inet_ntoa(*(struct in_addr *) &addr.ip), addr.port, info.client.username);
					sync_user(socket, user_dir, addr);
					pthread_mutex_unlock(&((((CLIENT_MUTEX_AND_BACKUP *)(found->data))->client_mutex).sync_or_command));
					
					break;
				case UPLOAD:
					pthread_mutex_lock(&((((CLIENT_MUTEX_AND_BACKUP *)(found->data))->client_mutex).sync_or_command));
					file_info = *((FILE_INFO*)cmd->argument);
                    printf("📝 [%s:%d] %s: SYNC uploading %s...		", inet_ntoa(*(struct in_addr *) &addr.ip), addr.port, info.client.username, file_info.filename);
					fflush(stdout);
					receive_file(file_info, user_dir, socket);
					printf("✅ OK!\n");
					
					REMOTE_ADDR *backup_cmd_servers;
					backup_cmd_servers =((CLIENT_MUTEX_AND_BACKUP*)(found->data))->backup_addresses;
					char file_path[MAX_PATH_LENGTH];
					strcpy(file_path,user_dir);
					strcat(file_path,file_info.filename);
					//Envia arquivo recebido para todos os servidores de backup
					for(i = 0; i < n_backup_servers; i++){
						send_file(backup_cmd_servers[i],file_path);
					}
					
					pthread_mutex_unlock(&((((CLIENT_MUTEX_AND_BACKUP *)(found->data))->client_mutex).sync_or_command));
                   
                    break;
                case DOWNLOAD:
					pthread_mutex_lock(&((((CLIENT_MUTEX_AND_BACKUP *)(found->data))->client_mutex).sync_or_command));
                    if(strlen((*cmd).argument) > 0){
                        printf("📝 [%s:%d] %s: SYNC downloading %s...	", inet_ntoa(*(struct in_addr *) &addr.ip), addr.port, info.client.username, cmd->argument);		
						fflush(stdout);
						strcpy(download_file_path, user_dir);
						strcat(download_file_path, cmd->argument);		
						send_file(addr, download_file_path);
						printf("✅ OK!\n");	
                    }else
                        printf("ERROR: download missing argument\n"); 
					pthread_mutex_unlock(&((((CLIENT_MUTEX_AND_BACKUP *)(found->data))->client_mutex).sync_or_command));

                    break;
                case DELETE:
					pthread_mutex_lock(&((((CLIENT_MUTEX_AND_BACKUP *)(found->data))->client_mutex).sync_or_command));
                    if(strlen((*cmd).argument) > 0){
                        printf("📝 [%s:%d] %s: SYNC deleting %s...		", inet_ntoa(*(struct in_addr *) &addr.ip), addr.port, info.client.username, cmd->argument);
						fflush(stdout);
						if(delete(cmd->argument, user_dir) < 0)
							printf("Error deleting file : %s\n", strerror(errno));
						
						REMOTE_ADDR *backup_cmd_servers;
						backup_cmd_servers =((CLIENT_MUTEX_AND_BACKUP*)(found->data))->backup_addresses;
						for(i = 0; i < n_backup_servers; i++){
							deleteFile(cmd->argument,backup_cmd_servers[i]);
						}
						
						printf("✅ OK!\n");
                    }else
                        printf("ERROR: delete missing argument\n");
					pthread_mutex_unlock(&((((CLIENT_MUTEX_AND_BACKUP *)(found->data))->client_mutex).sync_or_command));
                    
                    break;
			}
				
			
        }else
			printf("✉ [%s:%d] WARNING: Message ignored by SYNC thread. Wrong type.\n", inet_ntoa(*(struct in_addr *) &addr.ip), addr.port);
    }
}

void *thread_backup_cmd(void *thread_info){
	THREAD_INFO info = *(THREAD_INFO*)thread_info;
	char user_dir[MAX_PATH_LENGTH];
	REMOTE_ADDR addr = info.client.client_addr;
	PACKET msg;
	COMMAND *cmd;
	FILE_INFO file_info;
	struct sockaddr_in cli_addr;
	int n, socket = info.sock_cmd;

	cli_addr.sin_family = AF_INET;
    cli_addr.sin_port = htons(addr.port);
    cli_addr.sin_addr.s_addr = addr.ip;
    bzero(&(cli_addr.sin_zero), 8);

	// Path da pasta do usuário no servidor
	strcpy(user_dir, SERVER_DIR);	
	strcat(user_dir, info.client.username);
	strcat(user_dir, "/");
	
	create_user_dir(info.client.username);

	while(1){
		n = recv_packet(socket, &addr, &msg, 0);

		if (n < 0){
			printf("ERROR recvfrom 3:  %s\n", strerror(errno));
			pthread_exit(NULL);
		}

		cmd = (COMMAND *) &msg.data;

        if(msg.header.type == CMD){
			switch((*cmd).code){
				case UPLOAD:
					file_info = *((FILE_INFO*)cmd->argument);
                    printf("📝 [%s:%d] %s: BACKUP uploading %s...		", inet_ntoa(*(struct in_addr *) &addr.ip), addr.port, info.client.username, file_info.filename);
					fflush(stdout);
					receive_file(file_info, user_dir, socket);
					printf("✅ OK!\n");       
                    break;

                case DELETE:
                    if(strlen((*cmd).argument) > 0){
                        printf("📝 [%s:%d] %s: BACKUP deleting %s...		", inet_ntoa(*(struct in_addr *) &addr.ip), addr.port, info.client.username, cmd->argument);
						fflush(stdout);
						if(delete(cmd->argument, user_dir) <0)
							printf("Error deleting file : %s\n", strerror(errno));
						printf("✅ OK!\n");
                    }else
                        printf("ERROR: delete missing argument\n");
                    break;
			}
				
			
        }else
			printf("✉ [%s:%d] WARNING: Message ignored by Backup thread. Wrong type.\n", inet_ntoa(*(struct in_addr *) &addr.ip), addr.port);
    }

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

	pthread_create(&thr_sync, NULL, thread_client_sync, &thread_info);
	thread_info.tid_sync = thr_sync;
	pthread_create(&thr_cmd, NULL, thread_client_cmd, &thread_info);

	if(reply_hello(conn_info, listen_socket) < 0){
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

		if(n_packets > 0)
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

void list_backup_servers() {
	int i;
	printf("\n");
	for(i=0; i < n_backup_servers; i++) 
		printf("💾  BACKUP SERVER %d: %s:%d \n", i, inet_ntoa(*(struct in_addr *) &backup_servers[i].ip), backup_servers[i].port);
}

int send_backup_hello() {
    PACKET packet;

    packet.header.type = BACKUP;

    if (send_packet(backup_socket, main_server, packet, 0) < 0){
        fprintf(stderr, "ERROR! BACKUP HELLO failed\n");
        return -1;
    }

    return 0;
}

void sort_addr_list(REMOTE_ADDR *servers_list, int size) {
	for (int i=0; i < size; i++) {
		for (int j=0; j < size - 1; j++) {
			if ((servers_list[j].ip > servers_list[j+1].ip) || ((servers_list[j].ip == servers_list[j+1].ip) && (servers_list[j].port > servers_list[j+1].port))) {
				REMOTE_ADDR temp = servers_list[j];
				servers_list[j] = servers_list[j+1];
				servers_list[j+1] = temp;
			}
		}
	}
}

int delete_addr_list_index(int index, REMOTE_ADDR *list, int *size) {
	int found = -1;

	for (int i=0; i < *size; i++) {
		if (found == 0)
			list[i - 1] = list[i];
		else if (i == index) 
			found = 0;
	}

	if (found == 0)
		(*size)--;

	return found;
}

int update_backup_lists(int socket) {
	int i;
	PACKET packet;
	REMOTE_ADDR thisBackup;

	sort_addr_list(backup_servers, n_backup_servers);

	packet.header.type = BACKUP;

	for(i=0; i < n_backup_servers; i++){
		thisBackup = backup_servers[i];

		// Estrutura da msg:
		// [ INT n_backup_servers ][ INT i ][ REMOTE_ADDR backup_servers[] ]

		memcpy(packet.data, &n_backup_servers, sizeof(int));
		memcpy(packet.data + sizeof(int), &i, sizeof(int));
		memcpy(packet.data + sizeof(int)*2, backup_servers, sizeof(REMOTE_ADDR) * n_backup_servers);
		
		if(send_packet(socket, thisBackup, packet, DEFAULT_TIMEOUT) < 0)
			printf("%s:%d couldn't be reached\n", inet_ntoa(*(struct in_addr *) &thisBackup.ip), thisBackup.port);
	}

	return 0;
}

int declare_main_server(int socket) {
	int i;
	PACKET msg;

	pthread_cancel(thr_alive);

	printf("⭐  I am the new main server!\n");

	backup_mode = 0;
	//msg.header.type = CLOSE;
	//printf("Enviando CLOSE para %s:%d\n", inet_ntoa(*(struct in_addr *) &backup_servers[backup_index].ip), backup_servers[backup_index].port);
	//if(send_packet(socket, backup_servers[backup_index], msg, 0) < 0)
		//printf("ERROR sending CLOSE packet to backup_mode...\n");


	msg.header.type = NEW_LEADER;
	// Informa os demais backup_servers
	for (i=0; i < n_backup_servers; i++){
		if (i != backup_index) {
			//printf("Sending NEW_LEADER to %s:%d\n",  inet_ntoa(*(struct in_addr *) &backup_servers[i].ip), backup_servers[i].port);
			if(send_packet(socket, backup_servers[i], msg, DEFAULT_TIMEOUT) < 0)
				printf("Msg NEW_LEADER perdida...\n");
		}
	}

	sem_wait(&sem_election);

	msg.header.type = FRONT_END;
	memcpy(msg.data, &port, sizeof(int));

	// Avisa os dispositivos (clientes) sobre o novo server principal, requisitando
	//que façam login novamente
	for(i=0; i < n_devices; i++){
		//printf("Enviando request de hello para device %s:%d\n", inet_ntoa(*(struct in_addr *) &connected_devices[i].ip), connected_devices[i].port);
		if(send_packet(socket, connected_devices[i], msg, 1000) < 0)
			printf("Msg request hello to device perdida...\n");
	}

	delete_addr_list_index(backup_index, backup_servers, &n_backup_servers);
	resetDevicesList();
	update_backup_lists(socket);

	return 0;
}

int send_election_msg(int socket, REMOTE_ADDR server, int msec_timeout) {
	PACKET msg;
	msg.header.type = ELECTION;
	return send_packet(socket, server, msg, msec_timeout);
}

void *start_election() {
	int i;
	int election_socket = create_udp_socket();

	for (i = backup_index + 1; i < n_backup_servers; i++) {
		if(i != backup_index) {
			//printf("Sending ELECTION to %s:%d\n", inet_ntoa(*(struct in_addr *) &backup_servers[i].ip), backup_servers[i].port);
			if(send_election_msg(election_socket, backup_servers[i], DEFAULT_TIMEOUT) < 0)
				printf("Msg ELECTION perdida...\n");
			else
				return NULL;
		}
	}

	declare_main_server(election_socket);

	close(election_socket);
	return NULL;
}

void *is_server_alive(){
	PACKET msg;
	int alive_socket = create_udp_socket();
	pthread_t thr_election;

	msg.header.type = ALIVE;

	while(1) {
		sleep(alive_delay);

		alive_addr.ip = main_server.ip;
		alive_addr.port = ALIVE_PORT;

		if (electing == 0) {
			//printf("Are you alive server?\n");
			if(send_packet(alive_socket, alive_addr, msg, DEFAULT_TIMEOUT) < 0) {
				if(send_packet(alive_socket, alive_addr, msg, DEFAULT_TIMEOUT) < 0) {
					//printf("No response to ALIVE from %s:%d\n",  inet_ntoa(*(struct in_addr *) &alive_addr.ip), alive_addr.port);
					printf("🚨  Main server is down! Starting election\n");
					if (electing == 0) {
						electing = 1;		
						pthread_create(&thr_election, NULL, start_election, NULL);
					}
				}
			}
		}
		alive_delay = ALIVE_DELAY;
	}

	return NULL;
}

/*	DEVE SER CHAMADA APENAS QUANDO UM NOVO SERVER ASSUME.
	Desaloca memória associada a lista de devices e seta o número para 0.
*/
void resetDevicesList(){
	free(connected_devices);
	connected_devices = malloc(sizeof(REMOTE_ADDR));
	n_devices = 0;
}

int new_backup_user(CLIENT_INFO* backup_info){
	THREAD_INFO thread_info;
	CONNECTION_INFO conn_info;
	int socket_usr_backup;
	pthread_t usr_backup;

	socket_usr_backup = create_udp_socket();

	if(socket_usr_backup < 0){
		printf("Erro ao criar socket para backup do usuário %s", backup_info->username);
	}

	bind_udp_socket(socket_usr_backup, INADDR_ANY, 0);
	//Preenche informações de conexão à serem mandadas pro server principal
	conn_info.client = *(backup_info);
	conn_info.ports.port_cmd = get_socket_port(socket_usr_backup);
	conn_info.ports.port_sync =  0;
	//Preenche informações da thread
	thread_info.tid_sync = 0;
	thread_info.sock_cmd = socket_usr_backup;
	thread_info.client = *(backup_info);
	thread_info.sock_sync = 0;

	pthread_create(&usr_backup, NULL, thread_backup_cmd, (void *)&thread_info);

	if(reply_hello(conn_info,backup_socket) < 0){
		printf("ERROR responding HELLO message\n");
		return -1;
	}

	return 0;
}

void *run_backup_mode() {
	PACKET msg;
	REMOTE_ADDR rem_addr;
	CLIENT_INFO backup_info;
	REMOTE_ADDR device_to_delete;
	int index_to_delete;	
	pthread_t thr_election;

	pthread_create(&thr_alive, NULL, is_server_alive, NULL);

	// Cria o socket UDP para conexão de novos clientes
    backup_socket = create_udp_socket();
    backup_socket = bind_udp_socket(backup_socket, INADDR_ANY, port);

	send_backup_hello(); // Conecta com o servidor principal

	while (1) {
		if (recv_packet(backup_socket, &rem_addr, &msg, 0) < 0)
			printf("ERROR recv_packet run_backup_mode: %s\n", strerror(errno));
			
		switch (msg.header.type) {
			case HELLO:
				//Preenche backup-info: Username e remote_addr do server principal
				backup_info.client_addr = rem_addr;
				strcpy(backup_info.username, (char *) msg.data);

				if(new_backup_user(&backup_info) < 0)
					printf("ERROR creating backup socket and thread\n");

				//printf("New client connected: %s\n", backup_info.username);
				break;

			case BACKUP:
				n_backup_servers = (int) *(msg.data);
				backup_index = (int) *(msg.data + sizeof(int));
				backup_servers = malloc(sizeof(REMOTE_ADDR) * n_backup_servers);
				memcpy(backup_servers, msg.data + sizeof(int)*2, sizeof(REMOTE_ADDR) * n_backup_servers);
				printf("Backups list updated:");
				//list_backup_servers();
				electing = 0;
				break;

			case ELECTION:
				if (electing == 0) {
					electing = 1;
					pthread_create(&thr_election, NULL, start_election, NULL);
				}
				break;

			case NEW_DEVICE:
				n_devices++;
				connected_devices = realloc(connected_devices, sizeof(REMOTE_ADDR) * n_devices);
				connected_devices[n_devices-1] = *((REMOTE_ADDR*) msg.data);
				//printf("New device [%s:%d] has logged\n", inet_ntoa(*(struct in_addr *) &(((REMOTE_ADDR *) msg.data)->ip)), ((REMOTE_ADDR*)msg.data)->port);
				break;

			case NEW_LEADER:
				electing = 1;
				alive_delay = 4;
				main_server.ip = rem_addr.ip;
				alive_addr.ip = rem_addr.ip;
				main_server.port = PORT;
				// Restart alive thread
				pthread_cancel(thr_alive);
				pthread_create(&thr_alive, NULL, is_server_alive, NULL);
				resetDevicesList();
				printf("⭐  %s:%d is the new main server!\n", inet_ntoa(*(struct in_addr *) &rem_addr.ip), main_server.port);
				break;

			case DELETE_DEVICE:
				device_to_delete = *((REMOTE_ADDR *)msg.data);			
				
				index_to_delete = find_device_index(connected_devices,n_devices,device_to_delete);
				delete_addr_list_index(index_to_delete,connected_devices,&n_devices);
				
				//printf("New device [%s:%d] has been deleted\n", inet_ntoa(*(struct in_addr *) &(((REMOTE_ADDR *) msg.data)->ip)), ((REMOTE_ADDR*)msg.data)->port);
				
				break;
			
			case CLOSE:
				return NULL;

			default:
				printf("📡 [%s:%d] WARNING: Message ignored by backup_socket: %x\n", inet_ntoa(*(struct in_addr *) &rem_addr.ip), rem_addr.port, msg.header.type);
		}		
	}

	return NULL;
}

// Thread to reply ALIVE messages
void *reply_alive(){
	int socket_alive;
	PACKET msg;
	REMOTE_ADDR addr;

	socket_alive = create_udp_socket();
	socket_alive = bind_udp_socket(socket_alive, INADDR_ANY, ALIVE_PORT);

	while (1) {
		recv_packet(socket_alive, &addr, &msg, 0);
		if (msg.header.type != ALIVE)
			printf("📡 [%s:%d] WARNING: Message ignored by socket_alive: %x\n", inet_ntoa(*(struct in_addr *) &addr.ip), addr.port, msg.header.type);
	}
}

int run_server_mode() {
	PACKET msg;
	REMOTE_ADDR rem_addr;
	REMOTE_ADDR device_addr;
	CLIENT_INFO client_info;
	int i, inform_device_socket;
	pthread_t thr_alive;

	pthread_create(&thr_alive, NULL, reply_alive, NULL);

	// Cria o socket UDP para conexão de novos clientes
    listen_socket = create_udp_socket();
    listen_socket = bind_udp_socket(listen_socket, INADDR_ANY, PORT);

	inform_device_socket = create_udp_socket();
	inform_device_socket = bind_udp_socket(inform_device_socket,INADDR_ANY,inform_device);

	if(listen_socket < 0)
        exit(0);

	while (1) {
		sem_post(&sem_election);

		if (recv_packet(listen_socket, &rem_addr, &msg, 0) < 0)
			printf("ERROR recv_packet listen_socket\n");

		sem_init(&file_is_created, 0, 0);

		switch (msg.header.type) {
			case HELLO:
				device_addr.ip = rem_addr.ip;
				device_addr.port = FRONT_END_PORT;

				// Seta informações de client_info.
				client_info.client_addr = rem_addr;
				strcpy(client_info.username, (char *) msg.data);

				//Independente do usuário, é necessário notificar um novo device para os servidores de backup
				for(i = 0; i < n_backup_servers; i++){
					send_new_device(inform_device_socket, backup_servers[i], &device_addr);
				}

				// HASH TABLE
				//add_user_to_hashtable(client_info);
				ENTRY user_to_search;
				ENTRY *user_retrieved;
				ENTRY *user_to_add;
				CLIENT_MUTEX new_mutex;
				int i;

				user_to_search.key = client_info.username;

				//Caso tenha achado um usuário, incrementa o número de usuários logados
				if ((user_retrieved = hsearch(user_to_search, FIND)) != NULL){
					(((CLIENT_MUTEX_AND_BACKUP *)(user_retrieved->data))->client_mutex.clients_connected)++;
					//printf("Clientes %s conectados: %d\n", client_info.username, (((CLIENT_MUTEX*) user_retrieved->data)->clients_connected));
				//Caso não haja nenhum usuário
				} else {
					//printf("Usuário não encontrado na hash table, criando nova entrada\n");
					//Inicializa a nova estrutura de mutex	
					new_mutex.clients_connected = 1;
					pthread_mutex_init(&(new_mutex.sync_or_command), NULL);

					user_to_add = malloc(sizeof(ENTRY));
					// Aloca novas variáveis para os novos clientes
					user_to_add->data = malloc(sizeof(CLIENT_MUTEX_AND_BACKUP));
					//Aloca a área da lista
					((CLIENT_MUTEX_AND_BACKUP*) (user_to_add->data))->backup_addresses = malloc(sizeof(REMOTE_ADDR) * n_backup_servers);

					//Preenche o client_mutex
					memcpy(&(((CLIENT_MUTEX_AND_BACKUP*) user_to_add->data)->client_mutex), &new_mutex, sizeof(new_mutex));

					//Hello para servidores de backup e inicializar uma thread para cada usuario
					for (i = 0; i < n_backup_servers; i++){
						REMOTE_ADDR new_backup_server_cmd;
						new_backup_server_cmd.ip = backup_servers[i].ip;
						REMOTE_ADDR new_backup_server_sync; //Na verdade não é usado, apenas para manter compatibilidade com a versão de server
						//printf("Enviando HELLO para %s:%d \n", inet_ntoa(*(struct in_addr *) &backup_servers[i].ip), backup_servers[i].port);
						if(hello(client_info.username, listen_socket, backup_servers[i], &new_backup_server_cmd, &new_backup_server_sync) < 0)
							printf("Msg de HELLO para %s:%d falhou...\n", inet_ntoa(*(struct in_addr *) &backup_servers[i].ip), backup_servers[i].port);
						//backup_adresses[i] = new_backup_server_cmd recebido
						*((((CLIENT_MUTEX_AND_BACKUP*)(user_to_add->data))->backup_addresses) + i) = new_backup_server_cmd;
					}

					user_to_add->key = client_info.username;
					//Adiciona no hash
					hsearch(*user_to_add, ENTER);

					free(user_to_add);
				}

				if(new_client(&client_info) < 0) {
					printf("ERROR creating client sockets\n");
					exit(0);
				}

				if(create_user_dir((char *) &(msg.data)) < 0) {
					printf("ERROR creating user dir\n");
					exit(0);
				}
				
				sem_post(&file_is_created);
				
				printf("📡 [%s:%d] HELLO: connected as %s\n", inet_ntoa(*(struct in_addr *) &rem_addr.ip), rem_addr.port,(char *) &(msg.data));
				break;

			case BACKUP:
				n_backup_servers++;
				backup_servers = realloc(backup_servers, sizeof(REMOTE_ADDR)*n_backup_servers);
				backup_servers[n_backup_servers - 1] = rem_addr;
				printf("💾  NEW BACKUP SERVER: %s:%d \n", inet_ntoa(*(struct in_addr *) &rem_addr.ip), rem_addr.port);

				update_backup_lists(listen_socket);
				break;

			default:
				printf("📡 [%s:%d] WARNING: Message ignored by listen_socket Type: %x\n", inet_ntoa(*(struct in_addr *) &rem_addr.ip), rem_addr.port, msg.header.type);
		};		
	}
}

int main(int argc, char *argv[]){
	int opt;
	struct hostent *main_host;

	// Processa os argumentos passados na linha de comando
	// -p 3000 -> Seta uma porta diferente da padrão
	// -b 192.168.1.25 -> BACKUP MODE, identifica que essa instancia opera inicialmente como backup,
	//o IP do main_server é passado como argumento
	while ((opt = getopt(argc, argv, "p:b:")) != -1) {
		switch (opt) {
			case 'p':
				port = atoi(optarg);
				break;

			case 'b':
				if ((main_host = gethostbyname((char *)optarg)) == NULL) {
					fprintf(stderr, "ERROR! No such host\n");
					break;
				}
				main_server.ip = *(unsigned long *) main_host->h_addr;
				main_server.port = PORT;
				backup_mode = 1;
				break;

			default: /* '?' */
				fprintf(stderr, "Usage: %s [-p port] [-b main_server_ip]\n", argv[0]);
				exit(EXIT_FAILURE);
		}
	}

	// Cria a hash table que receberá dados sobre os clientes ativos
	if (hcreate(NUM_OF_MAX_CONNECTIONS) < 0)
		printf("Error creating hash table: %s\n", strerror(errno));

	gethostname(hostname, sizeof(hostname));

	// Inicialização do semáforo que indica quando um servidor de backup que assume como main_server está pronto para operar
	sem_init(&sem_election, 0, 0);

	while (1){
		if(backup_mode){
			// FAZER AS COISAS DO BACKUP MODE AQUI
			printf("✅  Server running at %s:%d (BACKUP SERVER)\n", hostname, port);
			printf("    Main server: %s\n\n", inet_ntoa(*(struct in_addr *) &main_server.ip));
			pthread_create(&thr_backup, NULL, run_backup_mode, NULL);
			while(backup_mode);
		}else{
			// FAZER AS COISAS DO MAIN SERVER AQUI
			printf("✅  Server running at %s:%d (MAIN SERVER)\n\n", hostname, PORT);
			run_server_mode();
		}
	}
    
	hdestroy();

    return 0;
}

