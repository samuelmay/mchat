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

int start_listening(struct server_options *opts) {
	/* create socket and start listening for incoming connections */
	int s;
	if ((s = socket(AF_INET,SOCK_STREAM,0)) < 0) {
		error(EXIT_FAILURE,errno,"listening socket creation failed");
	}

	struct sockaddr_in server_addr;
	struct sockaddr *server_addr_p = (struct sockaddr *)&server_addr;
	socklen_t len = sizeof(struct sockaddr_in);
	bzero(server_addr_p,len);

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	/* this will bind to any free port */
	server_addr.sin_port = 0;

	if (bind(s,server_addr_p,len) < 0) {
		error(EXIT_FAILURE,errno,"failed to bind listening socket");
	}

	/* since we binded to any free port, get the actual port we binded to,
	 * and set the options we will send to the registration server */
	if (getsockname(s,server_addr_p,&len) < 0) {
		error(EXIT_FAILURE,errno,"could not get port from bind");
	}
	opts->local_port = server_addr.sin_port;
	opts->local_port_h = ntohs(server_addr.sin_port);
	
	if (listen(s,1) < 0) {
		error(EXIT_FAILURE,errno,"failed to listen on listening socket");
	}
	
	return s;
}


void server_accept(struct connection *c) {
	struct sockaddr_in client_addr;
	struct sockaddr *client_addr_p = (struct sockaddr *)&client_addr;
	bzero(client_addr_p,sizeof(struct sockaddr_in));


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
		if (user_list.user[i].ip_addr == client_addr.sin_addr.s_addr &&
		    user_list.user[i].tcp_port == client_addr.sin_port) {
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

int receive_message(struct connection *c,char message[INPUT_LEN]) {
	int retval;
	int len = sizeof(char)*INPUT_LEN;
	if (c->socket < 0) {
		printf_threadsafe("You're not connected to anyone!\n");
	} else {
		retval = recv(c->socket,message,len,0);
		if (retval == 0) {
			/* closed connection at remote end */
			return 0;
		} else if (retval < len) {
			error(0,errno,"failed to receive message.");
		}
		printf_threadsafe("\n%s says: %s",c->remote_user,message);
	}
	return 1;
}

void *receive_messages(void *arg) {
	struct connection *c = (struct connection *)arg;
	char buffer[INPUT_LEN];
	while (receive_message(c,buffer)) {
		bzero(buffer,INPUT_LEN*sizeof(char));
	}
	printf_threadsafe("\n%s closed the connection.\n", c->remote_user);
	return NULL;
}
