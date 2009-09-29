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
/* #include <sys/select.h> */
#include "chat.h"

void server_accept(struct connection *c) {
	/* if we return on an error before we connect, this will make sure the
	 * connection is still marked as unconnected. */
	c->socket = -1;

	printf("listening for incoming connections...\n");
	
	/* create socket */
	if ((c->socket = socket(AF_INET,SOCK_STREAM,0)) < 0) {
		error(0,errno,"socket creation failed");
		return;
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

	if (bind(c->socket,server_addr_p,sizeof(struct sockaddr_in)) < 0) {
		error(0,errno,"failed to bind socket");
		c->socket = -1;
		return;
	}

	if (listen(c->socket,1) < 0) {
		error(0,errno,"failed to listen on socket");
		c->socket = -1;
		return;
	}

	socklen_t client_addr_len = sizeof(struct sockaddr_in);
	int s;
	if ((s = accept(c->socket,client_addr_p,&client_addr_len)) < 0) {
		error(0,errno,"failed to accept connection");
		c->socket = -1;
		return;
	} else {
		/* use the newly created socket */
		c->socket = s;
	}

	/* reverse look up user details, using IP address. This doesn't work on
	 * localhost. */
        /* UNSAFE CONCURRENT STUFF BEGINS */	
	pthread_mutex_lock(&user_list_lock);
	int i;
	int found = 0;
	for (i = 0; i < ntohl(user_list.nusers) && i < 50; i++) {
		if (user_list.user[i].ip_addr == client_addr.sin_addr.s_addr) {
			found = 1;
			strncpy(c->remote_user,
				user_list.user[i].username,USERNAME_LEN);
			break;
		}
	}
	pthread_mutex_unlock(&user_list_lock);
	/* UNSAFE CONCURRENT STUFF ENDS */

	if (!found) {
		strncpy(c->remote_user,"unknown",USERNAME_LEN);
	}

	printf("accepted incoming connection from %s.\n",c->remote_user);

	pthread_t receive_messages_thread;
	pthread_create(&receive_messages_thread,
		       NULL,
		       receive_messages,
		       c);
	return;
}
