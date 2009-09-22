/*
 * registration.c
 *
 * Sam May
 * 22/09/09
 *
 * Thread that polls the server with registration messages
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
#include <pthread.h>
#include "chat.h"

/** global variables **
 **********************/
struct reg_resp user_list;
pthread_mutex_t user_list_lock = PTHREAD_MUTEX_INITIALIZER;


void print_user_list(void) {
	unsigned int i;
	char ip[INET_ADDRSTRLEN];
	char *username;
	unsigned long nusers;
	unsigned short port;

        /* UNSAFE CONCURRENT STUFF BEGINS */	
	pthread_mutex_lock(&user_list_lock); /* NUMBER 1 */
	pthread_mutex_lock(&console_lock);   /* NUMBER 2 */
	nusers = ntohl(user_list.nusers);
	printf("%14s %15s %6s\n","USERNAME","IP ADDRESS","PORT");
	for (i = 0; i < nusers && i < 50; i++) {
		username = (char *)&(user_list.user[i].username);
		port = ntohs(user_list.user[i].tcp_port);
		inet_ntop(AF_INET,&(user_list.user[i].ip_addr),ip,INET_ADDRSTRLEN);
		printf("%14s %15s %6d\n",username, ip, port);
	}
	pthread_mutex_unlock(&console_lock);   /* NUMBER 2 */
	pthread_mutex_unlock(&user_list_lock); /* NUMBER 1 */
	/* UNSAFE CONCURRENT STUFF ENDS */
	
	return;
}

void *poll_server(void *arg) {
	struct server_options *server_opts = arg; 
	int socket_fd;

	/* create socket */
	if ((socket_fd = socket(AF_INET,SOCK_DGRAM,0)) < 0) {
		error(EXIT_FAILURE,errno,"socket creation failed");
	}

	/* zero and assign server address struct  */
	struct sockaddr_in server_addr;
	struct sockaddr *server_addr_p = (struct sockaddr *)&server_addr;
	bzero(&server_addr,sizeof(struct sockaddr_in));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = server_opts->port;
	server_addr.sin_addr = server_opts->ip;


	/* compose and send our registration message */
	struct reg_msg message;
	struct reg_resp response;
	socklen_t response_len;  

	bzero(&message,sizeof(struct reg_msg));
	strncpy((char *)&message.password,server_opts->password,15);
	strncpy((char *)&message.username,server_opts->username,13);
	message.tcp_port = htons(CLIENT_PORT);

	/* poll server continously, every 30s */
	while (1) {
		/* printf("sending registration message to server..."); */
		if (sendto(socket_fd, &message, sizeof(struct reg_msg), 0,
			   server_addr_p,sizeof(struct sockaddr_in)) < 0) {
			error(EXIT_FAILURE,errno,"failed to send registration message to server");
		}

		if (recvfrom(socket_fd, &response, sizeof(struct reg_resp), 0,
			     server_addr_p, &response_len) < 0) {
			error(EXIT_FAILURE,errno,"failed to recieve response from server");
		}
		/* printf("recieved response!\n"); */

		/* UNSAFE CONCURRENT STUFF BEGINS */
		pthread_mutex_lock(&user_list_lock);
		memcpy(&user_list,&response,sizeof(struct reg_resp));
		pthread_mutex_unlock(&user_list_lock);
		/* UNSAFE CONCURRENT STUFF ENDS */

		sleep(SERVER_TIMEOUT/2);
	}
	return NULL;
}
