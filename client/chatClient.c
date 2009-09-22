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
#define USERNAME "Sam May"
#define CLIENT_PORT 31181
/* #define SERVER_IP "149.171.92.193" */
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 31180
#define SERVER_TIMEOUT 60

struct reg_msg {
	char passwd[16];
	char username[14];
	unsigned short tcp_port;
};

struct reg_resp {
	unsigned long nusers; /* number of users (no more than 50) */
	struct user_info {
		char username[14];
		unsigned short tcp_port;
		unsigned long ip_addr;
	} user[50]; /* info about each user */
};

void print_user_list(struct reg_resp *response);

int main (int argc, char **argv) {
	int socket_fd;

	printf("creating socket...\n");

	/* create socket */
	if ((socket_fd = socket(AF_INET,SOCK_DGRAM,0)) < 0) {
		error(EXIT_FAILURE,errno,"socket creation failed");
	}

	/* zero and assign server address struct  */
	struct sockaddr_in server_addr;
	bzero(&server_addr,sizeof(server_addr));

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT); /* htons IS IMPORTANT!!! */
	if (inet_aton(SERVER_IP,&server_addr.sin_addr) == 0) {
		error(EXIT_FAILURE,errno,"invalid server IP address");
	}

	/* compose and send our registration message */
	struct reg_msg message;
	struct reg_resp response;
	socklen_t response_len;  

	bzero(&message,sizeof(message));
	strncpy((char *)&message.passwd,PASSWORD,15);
	strncpy((char *)&message.username,USERNAME,13);
	message.tcp_port = htons(CLIENT_PORT);

	/* poll server continously, every 30s */
	while (1) {
		printf("sending registration message to server...");
		if (sendto(socket_fd, &message, sizeof(message), 0,
			   (struct sockaddr *)&server_addr,sizeof(server_addr)) < 0) {
			error(EXIT_FAILURE,errno,"failed to send registration message to server");
		}

		if (recvfrom(socket_fd, &response, sizeof(response), 0,
			     (struct sockaddr *)&server_addr, &response_len) < 0) {
			error(EXIT_FAILURE,errno,"failed to recieve response from server");
		}
		printf("recieved response!\n"); 

		print_user_list(&response);

		sleep(SERVER_TIMEOUT/2);
	}

	return 0;
}	

void print_user_list(struct reg_resp *response) {
	unsigned int i;
	char ip[INET_ADDRSTRLEN];
	char *username;
	unsigned long nusers = ntohl(response->nusers);
	unsigned short port;

	printf("%14s %15s %6s\n","USERNAME","IP ADDRESS","PORT");
	for (i = 0; i < nusers && i < 50; i++) {
		username = (char *)&(response->user[i].username);
		port = ntohs(response->user[i].tcp_port);
		inet_ntop(AF_INET,&(response->user[i].ip_addr),ip,INET_ADDRSTRLEN);
		printf("%14s %15s %6d\n",username, ip, port);
	}
	return;
}
