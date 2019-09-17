#include <stdio.h>
#include <stdlib.h>
#include "../include/fsync_server.h"
#include "../include/socket.h"

int listen_socket, n;
socklen_t clilen;
struct sockaddr_in cli_addr;
char buf[256];

int main(int argc, char const *argv[]){
    listen_socket = create_udp_socket();
    listen_socket = bind_udp_socket(listen_socket, INADDR_ANY, PORT);
    int ok = 0;

    if(listen_socket < 0)
        return -1;
 
    memset(&cli_addr, 0, sizeof(cli_addr));
    clilen = sizeof(struct sockaddr_in);

    while (1) {
		/* receive from socket */
		n = recvfrom(listen_socket, buf, 256, 0, (struct sockaddr *) &cli_addr, &clilen);
		if (n < 0) 
			printf("ERROR on recvfrom");
		printf("- Received a datagram: %s\n", buf);
		
		/* send to socket */
		n = sendto(listen_socket, &ok, sizeof(int), 0, (const struct sockaddr *) &cli_addr, clilen);
		if (n < 0) 
		    printf("ERROR sendto %d\n", errno);
	}

    return 0;
}
