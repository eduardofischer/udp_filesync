#include <stdio.h>
#include <stdlib.h>
#include "../include/server.h"
#include "../include/communication.h"

int main(int argc, char const *argv[]){
	int listen_socket, n;
	socklen_t clilen;
	struct sockaddr_in cli_addr;
	PACKET msg;

    listen_socket = create_udp_socket();
    listen_socket = bind_udp_socket(listen_socket, INADDR_ANY, PORT);

    if(listen_socket < 0)
        return -1;
 
    memset(&cli_addr, 0, sizeof(cli_addr));
    clilen = sizeof(struct sockaddr_in);

    while (1) {
		/* receive from socket */
		n = recvfrom(listen_socket, &msg, PACKET_SIZE, 0, (struct sockaddr *) &cli_addr, &clilen);
		if (n < 0) 
			printf("ERROR on recvfrom");
		printf("- Received a datagram: %s\n", (char *) &(msg.data));
		
		ack(listen_socket, (struct sockaddr *) &cli_addr, clilen);
	}

    return 0;
}
