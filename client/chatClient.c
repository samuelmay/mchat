/*
 * chatClient.c
 *
 * Sam May 3206842
 * 22/09/09
 *
 * Client/server for a simple UDP internet chat protocol. A TELE3118
 * mini-project.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <error.h>
#include <unistd.h>

#define PASSWORD "3118miniproject"
#define USERNAME "sammay"
/* #define SERVER_IP "149.171.92.193" */
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 31180
#define SERVER_TIMEOUT 60

struct reg_msg {
	char passwd[16];
	char username[14];
	unsigned short tcpPort;
};

int main (int argc, char **argv) {
	int socket_fd;
	struct sockaddr_in server_addr;
	/* create socket */
	if ((socket_fd = socket(AF_INET,SOCK_DGRAM,0)) < 0) {
		error(EXIT_FAILURE,errno,"socket creation failed");
	} else {
		printf("created socket\n");
	}

	/* zero and assign server address struct  */
	bzero(&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = SERVER_PORT;
	if (inet_pton(AF_INET,SERVER_IP,&server_addr.sin_addr) < 0) {
		error(EXIT_FAILURE,errno,"invalid server IP address");
	}

	if (connect(socket_fd,
		    (struct sockaddr *)&server_addr,
		    sizeof(server_addr)) < 0) {
		error(EXIT_FAILURE,errno,"could not connect to server");
	} else {
		printf("connected to server\n");
	}

	return 0;
}
