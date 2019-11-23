#include "../include/communication.h"
#include "../include/server.h"
#include "../include/filesystem.h"
#include <utime.h>

/**
 *  Inicializa um socket UDP
 * */
int create_udp_socket(){
    int sockfd;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        printf("ERROR opening socket\n");
        return -1;
    }

    return sockfd;
}

/** 
 *  Vincula um socket com um IP e uma porta
 * */
int bind_udp_socket(int socket, char *ip, unsigned int port){
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;         // IPv4
    addr.sin_port = htons(port);       // Porta
    addr.sin_addr.s_addr = INADDR_ANY; // IP
    bzero(&(addr.sin_zero), 8);

    if (bind(socket, (struct sockaddr *)&addr, sizeof(struct sockaddr)) < 0)
    {
        printf("ERROR on binding: %s\n", strerror(errno));
        return -1;
    }

    return socket;
}

uint16_t get_socket_port(int socket){
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    getsockname(socket, (struct sockaddr *) &addr, &addr_len);

    return ntohs(addr.sin_port);
}

/** 
 *  Envia uma mensagem e espera o ACK do destino
 *  Retorno: Porta do servidor
 *           -1 (Error)
 *           -2 (Time out)
 **/
int send_packet(int socket, REMOTE_ADDR addr, PACKET packet, int msec_timeout){
    struct sockaddr_in dest_addr, new_addr;
    socklen_t addr_len = sizeof(new_addr);
    PACKET response;

    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(addr.port);
    dest_addr.sin_addr.s_addr = addr.ip;
    bzero(&(dest_addr.sin_zero), 8);

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = msec_timeout * 1000;

    setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    int successfully_sent = -2, n_timeouts = 0;
    do {
        if(sendto(socket, &packet, PACKET_SIZE, 0, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_in)) < 0) {
            //printf("ERROR send_packet sendto: %s\n", strerror(errno));
            return -1;
        }

        if(recvfrom(socket, &response, sizeof(PACKET), 0, (struct sockaddr *)&new_addr, &addr_len) < 0 || response.header.type != ACK) {
            //printf("\nERROR send_packet recvfrom %s\n", strerror(errno));
            //printf("Resending packet.\n");

            n_timeouts++;
        }
        else{
            successfully_sent = 0;
        }
    } while(successfully_sent < 0 && n_timeouts < MAX_TIMEOUTS);

    // Retorna o timeout para infinito
    tv.tv_usec = 0;
    setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    return successfully_sent;
}

int recv_packet(int socket, REMOTE_ADDR *addr, PACKET *packet, int usec_timeout){
    struct sockaddr_in new_addr;
    socklen_t addr_len = sizeof(new_addr);
    int n;

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = (__suseconds_t) usec_timeout;

    setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv));

    n = recvfrom(socket, packet, sizeof(PACKET), 0, (struct sockaddr *)&new_addr, &addr_len);

    if (n < 0){
        //printf("Timeout recv_packet recvfrom %s\n", strerror(errno));
        return -1;
    }

    ack(socket, (struct sockaddr *)&new_addr, addr_len);

    if(addr != NULL){
        addr->ip = new_addr.sin_addr.s_addr;
        addr->port = ntohs(new_addr.sin_port);
    }
    
    return n;
}

/** 
 *  Envia um pacote de ACK 
 * */
int ack(int socket, struct sockaddr *cli_addr, socklen_t clilen){
    PACKET packet;
    int n;

    packet.header.type = ACK;
    packet.header.seqn = 0;
    packet.header.total_size = 1;
    packet.header.length = 0;

    memset(packet.data, 0, DATA_LENGTH);

    n = sendto(socket, &packet, sizeof(PACKET), 0, (struct sockaddr *)cli_addr, clilen);

    return n;
}

/** 
 *  Envia um comando genérico ao servidor e aguarda pelo ack do mesmo 
 * */
int send_command(int socket, REMOTE_ADDR server, char command, char* arg, int usec_timeout){
    PACKET packet;

    //Prepara o pacote de comando
    packet.header.type = CMD;
    packet.header.seqn = 0;
    packet.header.total_size = 1;
    packet.header.length = sizeof(COMMAND);

    (*(COMMAND *) &(packet.data)).code = command;
    if(arg != NULL){
        strcpy((*(COMMAND *) &(packet.data)).argument, arg);
    }else{
        strcpy((*(COMMAND *) &(packet.data)).argument, "");
    };
    //Envia o pacote
    return send_packet(socket, server, packet, usec_timeout);
}

/**
 *  Inicializa o pacote de dados a ser enviado para o servidor. 
 * **/
void init_data_packet_header(PACKET *toInit, uint32_t total_size){
    toInit->header.type = DATA;
    toInit->header.seqn = 0;
    toInit->header.total_size = total_size;
}

int send_upload(int socket, REMOTE_ADDR server, FILE_INFO *file_info){
    PACKET packet;

    //Prepara o pacote de comando
    packet.header.type = CMD;
    packet.header.seqn = 0;
    packet.header.total_size = 1;
    packet.header.length = sizeof(COMMAND);

    (*(COMMAND *) &(packet.data)).code = UPLOAD;
    
    //Copia file_info para o argumento de comando genérico, para não quebrar com a estrutura padrão.
    memcpy((*(COMMAND *) &(packet.data)).argument,file_info, sizeof(FILE_INFO));
    return send_packet(socket, server, packet, 0);
}

/** Envia o arquivo para o servidor **/
int send_file(REMOTE_ADDR address, char *filePath){
    int socketDataTransfer;
    int response, i;
    FILE *sourceFile;
    PACKET dataToTransfer;
    FILE_INFO file_info;
    struct stat file_stats;
    char buffer[DATA_LENGTH];
    char filename[FILE_NAME_SIZE];

    get_filename_from_path(filePath, filename);
    
    //Pega as estatísticas do arquivo e preenche a estrutura file_info
    stat(filePath,&file_stats);   
    strcpy(file_info.filename, filename);
    file_info.modification_time = file_stats.st_mtime;
    file_info.access_time = file_stats.st_atime;

    socketDataTransfer = create_udp_socket();

    if (socketDataTransfer != ERR_SOCKET){
        response = send_upload(socketDataTransfer, address, &file_info);
        if(response >= 0){
             sourceFile = fopen(filePath,"rb");
             if(isOpened(sourceFile)){       
                int sourceFileSize = fileSize(sourceFile); 
                int sourceFileSizeRemaining = sourceFileSize;
                int currentPacketLenght;
                init_data_packet_header(&dataToTransfer,(u_int32_t)fileSizeInPackets(sourceFileSize));
              
                for(i = 0; i < fileSizeInPackets(sourceFileSize); i++){
                    //Preenchimento do pacote: dados e cabeçalho
                    currentPacketLenght = (sourceFileSizeRemaining / DATA_LENGTH) >= 1? DATA_LENGTH : sourceFileSizeRemaining % DATA_LENGTH;
                    fread(buffer,sizeof(char),DATA_LENGTH,sourceFile);
                    memcpy(dataToTransfer.data,buffer,currentPacketLenght);
                    dataToTransfer.header.seqn = i;
                    dataToTransfer.header.length = currentPacketLenght;
                            
                    //Enquanto o servidor não recebeu o pacote, tenta re-enviar pra ele
                    while(send_packet(socketDataTransfer,address,dataToTransfer,0) < 0);
                    
                    //Tamanho restante a ser transmitido
                    sourceFileSizeRemaining -= currentPacketLenght;
                }
                fclose(sourceFile);
                return SUCCESS;
             }
             return ERR_OPEN_FILE;
        } 
        else{
            printf("Server didn't return ack (busy?)\n");
            return ERR_ACK;
        }
    }

    return ERR_SOCKET; 
}


int receive_file(FILE_INFO file_info, char *dir_path, int dataSocket){
	FILE *toBeCreated;
	char file_path[MAX_PATH_LENGTH];
    char temp_file_path[MAX_PATH_LENGTH];
	int last_received_packet, final_packet, n, first_message_not_received = 1;
	PACKET received;
	struct utimbuf time;

	strcpy(temp_file_path, dir_path);
	strcat(temp_file_path, "~");
	strcat(temp_file_path, file_info.filename);

	// Timestamps novos
	time.actime = file_info.access_time;
	time.modtime = file_info.modification_time;

	// Mount file path
	strcpy(file_path, dir_path);
	strcat(file_path, file_info.filename);
	
	toBeCreated = fopen(file_path, "wb");
	
	if(isOpened(toBeCreated)){
        // Remove o arquivo temporário caso exista
        remove(temp_file_path);

		do{
            n = recv_packet(dataSocket, NULL, &received, 0);
			if (n < 0){
				printf("Error recv_packet at receive_file: %s", strerror(errno));
			}
			else{ //Message correctly received
				first_message_not_received = 0; //The first message was received
				write_packet_to_the_file(&received,toBeCreated);
                final_packet = received.header.total_size - 1;
			    last_received_packet = received.header.seqn;
			}
		}while(first_message_not_received);

	
		while(last_received_packet < final_packet){
			n = recv_packet(dataSocket, NULL, &received, 0);
			if (n < 0){
				printf("Error recv_packet at receive_file: %s", strerror(errno));
			} else{
                write_packet_to_the_file(&received,toBeCreated);
			    last_received_packet = received.header.seqn;
            }
		}
		
		fclose(toBeCreated);
		utime(file_path, &time);
		
		return SUCCESS;
	}
	else{
		printf("Erro ao criar arquivo em receive_file, tentou-se criar o arquivo: %s", file_path);
		return ERR_OPEN_FILE;
	}
}

/** Pede que o servidor exclua um arquivo da pasta do usuario **/
int deleteFile(char* fileName, REMOTE_ADDR remote){
    int socketDataTransfer;
    int response;

    socketDataTransfer = create_udp_socket();

    if (socketDataTransfer != ERR_SOCKET){
        response = send_command(socketDataTransfer, remote, DELETE, fileName, 0);
        
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

int write_packet_to_the_file(PACKET *packet, FILE *file){
    int bytes_written;
    //Sets the pointer
    fseek(file,packet->header.seqn * DATA_LENGTH, SEEK_SET);
    //Writes the pointer to the file
    bytes_written = fwrite(packet->data,sizeof(char), packet->header.length,file);
    rewind(file);

    if (bytes_written == sizeof(char) * packet->header.length){
        return SUCCESS;
    }
    else{
        printf("There was a error writing to the file.");
        return ERR_WRITING_FILE;
    }

}

/** 
 *  Inicia a comunicação de um cliente com o servidor 
 *  Retorna a porta com a qual o cliente deve se comunicar
 *  ou -1 em caso de erro
*/
int request_hello(char *username, int socket, REMOTE_ADDR destination, REMOTE_ADDR *cmd_address, REMOTE_ADDR *sync_address){
    PACKET packet, response;
    int n;

    packet.header.type = HELLO;
    strcpy((char *)&(packet.data), username);

    n = send_packet(socket, destination, packet, 0);

    if (n < 0){
        fprintf(stderr, "ERROR! HELLO failed\n");
        return -1;;
    }

    if(recv_packet(socket, NULL, &response, 0) < 0){
        printf("ERROR recv_packet\n");
        return -1;
    }

    if(response.header.type == HELLO){
        cmd_address->port = ((SERVER_PORTS_FOR_CLIENT *)&response.data)->port_cmd;
        sync_address->port = ((SERVER_PORTS_FOR_CLIENT *)&response.data)->port_sync;
    }

    return 0;
}

int answer_hello(CONNECTION_INFO conn, int listen_socket){
	PACKET packet;

	packet.header.type = HELLO;
	memcpy(&packet.data, &conn.ports, sizeof(SERVER_PORTS_FOR_CLIENT));

	return send_packet(listen_socket, conn.client.client_addr, packet, 0);
}