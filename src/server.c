#include <stdio.h>
#include <stdlib.h>
#include "../include/server.h"
#include "../include/communication.h"
#include "../include/filesystem.h"

/** 
 *  Escuta um cliente em um determinado socket 
 * */
void *listen_to_client(void *client){
    int new_socket, n;
    PACKET msg;
    REMOTE_ADDR addr = *(REMOTE_ADDR *) client;
    struct sockaddr_in cli_addr;
	socklen_t clilen = sizeof(cli_addr);
    COMMAND *cmd;

    new_socket = create_udp_socket();

    if(new_socket < 0){
        printf("ERROR creating new socket\n");
        exit(0);
    }

    cli_addr.sin_family = AF_INET;
    cli_addr.sin_port = htons(addr.port);
    cli_addr.sin_addr.s_addr = addr.ip;
    bzero(&(cli_addr.sin_zero), 8);

    n = ack(new_socket, (struct sockaddr *)&cli_addr, sizeof(struct sockaddr_in));
    if (n < 0){
        printf("Error ack %d/n", errno);
    }

    while(1){
		n = recvfrom(new_socket, &msg, PACKET_SIZE, 0, (struct sockaddr *) &cli_addr, &clilen);

		if (n < 0) 
			printf("ERROR on recvfrom");

        if(msg.header.type == CMD){
            cmd = (COMMAND *) &msg.data;

            switch((*cmd).code){
                case UPLOAD:
                    if(strlen((*cmd).argument) > 0){
                        printf("📝 [%s:%d] CMD: UPLOAD %s\n", inet_ntoa(*(struct in_addr *) &addr.ip), addr.port, (*cmd).argument);
                        ack(new_socket, (struct sockaddr *) &cli_addr, clilen);
                    }else
                        err(new_socket, (struct sockaddr *) &cli_addr, clilen, "UPLOAD missing argument");      

                    break;
                case DOWNLOAD:
                    if(strlen((*cmd).argument) > 0){
                        printf("📝 [%s:%d] CMD: DOWNLOAD %s\n", inet_ntoa(*(struct in_addr *) &addr.ip), addr.port, (*cmd).argument);
                        ack(new_socket, (struct sockaddr *) &cli_addr, clilen);
                    }else
                        err(new_socket, (struct sockaddr *) &cli_addr, clilen, "DOWNLOAD missing argument"); 
                    break;
                case DELETE:
                    if(strlen((*cmd).argument) > 0){
                        printf("📝 [%s:%d] CMD:: DELETE %s\n", inet_ntoa(*(struct in_addr *) &addr.ip), addr.port, (*cmd).argument);
                        ack(new_socket, (struct sockaddr *) &cli_addr, clilen);
                    }else
                        err(new_socket, (struct sockaddr *) &cli_addr, clilen, "DELETE missing argument");
                    
                    break;
                case LST_SV:
                    printf("📝 [%s:%d] CMD: LST_SV\n", inet_ntoa(*(struct in_addr *) &addr.ip), addr.port);
                    ack(new_socket, (struct sockaddr *) &cli_addr, clilen);
                    break;
                case SYNC_DIR:
                    printf("📝 [%s:%d] CMD: SYNC_DIR\n", inet_ntoa(*(struct in_addr *) &addr.ip), addr.port);
                    ack(new_socket, (struct sockaddr *) &cli_addr, clilen);
                    break;
                case EXIT:
                    printf("📝 [%s:%d] CMD: EXIT\n", inet_ntoa(*(struct in_addr *) &addr.ip), addr.port);
                    ack(new_socket, (struct sockaddr *) &cli_addr, clilen);
					pthread_exit(NULL);
                    break;
                default:
                    fprintf(stderr, "❌ ERROR Invalid Command\n");
                    err(new_socket, (struct sockaddr *) &cli_addr, clilen, "Invalid command");
            }
        }else if(msg.header.type == DATA)
			printf("✉ [%s:%d] DATA: %s\n", inet_ntoa(*(struct in_addr *) &addr.ip), addr.port,(char *) &(msg.data));
		
    }
}

/** 
 *  Cria um novo socket em uma nova thread
 *  Retorna a porta do novo socket ou -1 em caso de erro
 */
int new_socket(REMOTE_ADDR *client){
    pthread_t thr;       /* thread descriptor */

    return pthread_create(&thr, NULL, listen_to_client, client);
}

int main(int argc, char const *argv[]){
	int listen_socket, new_sock, n;
	struct sockaddr_in cli_addr;
	socklen_t clilen = sizeof(cli_addr);
	PACKET msg;
	REMOTE_ADDR client;

    listen_socket = create_udp_socket();
    listen_socket = bind_udp_socket(listen_socket, INADDR_ANY, PORT);

    if(listen_socket < 0)
        return -1;
 
    memset(&cli_addr, 0, sizeof(cli_addr));
    clilen = sizeof(struct sockaddr_in);

    while (1) {
		n = recvfrom(listen_socket, &msg, PACKET_SIZE, 0, (struct sockaddr *) &cli_addr, &clilen);
		
		if (n < 0) 
			printf("ERROR on recvfrom");

		client.ip = cli_addr.sin_addr.s_addr;
		client.port = ntohs(cli_addr.sin_port);

		new_sock = new_socket(&client);

		if(new_sock < 0){
			printf("ERROR creating socket\n");
			exit(0);
		}

		if (create_user_dir((char *) &(msg.data)) < 0) 
			exit(0);
		
		printf("📡 [%s:%d] HELLO: connected as %s\n", inet_ntoa(*(struct in_addr *) &client.ip), client.port,(char *) &(msg.data));	
	}

    return 0;
}
