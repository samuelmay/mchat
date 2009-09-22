/*
 * server.c
 *
 * Sam May
 * 22/09/09
 *
 * This file contains the code for accepting a connection from another chat
 * user.
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
#include <pthread.h>
#include "chat.h"

int server_accept(char *user) {
	printf("listening for incoming connections...\n");
	
	/* create a socket */
	int s1;
	if ((s1 = socket(AF_INET,SOCK_STREAM,0)) < 0) {
		error(0,errno,"socket creation failed");
		return -1;
	}

	struct sockaddr_in server_addr;
	struct sockaddr *server_addr_p = (struct sockaddr *)&server_addr;
	bzero(server_addr_p,sizeof(struct sockaddr_in));

	struct sockaddr_in client_addr;
	struct sockaddr *client_addr_p = (struct sockaddr *)&client_addr;
	bzero(client_addr_p,sizeof(struct sockaddr_in));

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(CLIENT_PORT);

	if (bind(s1,server_addr_p,sizeof(struct sockaddr_in)) < 0) {
		error(0,errno,"failed to bind socket");
		return -1;
	}

	if (listen(s1,1) < 0) {
		error(0,errno,"failed to listen on socket");
		return -1;
	}

	socklen_t client_addr_len = sizeof(struct sockaddr_in);
	int s2;
	if ((s2 = accept(s1,client_addr_p,&client_addr_len)) < 0) {
		error(0,errno,"failed to accept connection");
		return -1;
	}

	/* reverse look up user details, using IP address */
        /* UNSAFE CONCURRENT STUFF BEGINS */	
	pthread_mutex_lock(&user_list_lock);
	int i;
	int found = 0;
	for (i = 0; i < ntohl(user_list.nusers) && i < 50; i++) {
		if (user_list.user[i].ip_addr == client_addr.sin_addr.s_addr) {
			found = 1;
			strncpy(user,user_list.user[i].username,USERNAME_LEN);
			break;
		}
	}
	pthread_mutex_unlock(&user_list_lock);
	/* UNSAFE CONCURRENT STUFF ENDS */

	if (!found) {
		strncpy(user,"unknown",USERNAME_LEN);
	}

	return s2;
}
