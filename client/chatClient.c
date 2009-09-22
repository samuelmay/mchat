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
#include <errno.h>
#include <error.h>
#include <unistd.h>

#define PASSWORD "3118miniproject"
#define USERNAME "sammay"
#define SERV_TIMEOUT 60

struct reg_msg {
	char passwd[16];
	char username[14];
	unsigned short tcpPort;
};

int main (int argc, char **argv) {
	int sock;

	if ((sock = socket(AF_INET,SOCK_DGRAM,0)) < 0) {
		error(EXIT_FAILURE,errno,"socket creation failed");
	}
	
	return 0;
}
