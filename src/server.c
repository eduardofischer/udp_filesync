#include <stdio.h>
#include <stdlib.h>
#include "../include/server.h"
#include "../include/communication.h"

#define MAX_PATH_LENGTH 255

void upload(char *archive_name, char *archive_file){
	FILE *toBeCreated;
	char *full_archive_path = malloc(sizeof(char) * MAX_PATH_LENGTH);
	toBeCreated = fopen(archive_name,"")
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

		printf("%s:%d connected as %s\n", inet_ntoa(*(struct in_addr *) &client.ip), client.port,(char *) &(msg.data));
	}

    return 0;
}
