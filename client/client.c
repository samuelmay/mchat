/*
 * client.c
 *
 * Sam May
 * 22/09/09
 *
 * This file contains the code for connecting to another chat user via TCP and
 * exchanging messages.
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

int client_connect(char *user) {
	printf("connecting to user '%s'\n",user);

	struct sockaddr_in server_addr;
	struct sockaddr *server_addr_p = (struct sockaddr *)&server_addr;
	bzero(server_addr_p,sizeof(struct sockaddr_in));
	/* look up user details */
        /* UNSAFE CONCURRENT STUFF BEGINS */	
	pthread_mutex_lock(&user_list_lock);
	int i;
	int found = 0;
	for (i = 0; i < ntohl(user_list.nusers) && i < 50; i++) {
		if (strncmp(user_list.user[i].username,
			    user,
			    USERNAME_LEN) == 0) {
			found = 1;
			server_addr.sin_family = AF_INET;
			server_addr.sin_port = user_list.user[i].tcp_port;
			server_addr.sin_addr.s_addr = 
				user_list.user[i].ip_addr;
			break;
		}
	}
	pthread_mutex_unlock(&user_list_lock);
	/* UNSAFE CONCURRENT STUFF ENDS */

	if (!found) {
		printf("That user's not logged in!\n");
		return -1;
	}

	/* create a socket */
	int s;
	if ((s = socket(AF_INET,SOCK_STREAM,0)) < 0) {
		error(0,errno,"socket creation failed");
		return -1;
	}

	/* connect to the server */
	if (connect(s,server_addr_p,sizeof(struct sockaddr_in)) < 0) {
		error(0,errno,"could not connect to server");
		return -1;
	}

	return s;
}
