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

void client_connect(struct connection *c) {
	/* if we return on an error before we connect, this will make sure the
	 * connection is still marked as unconnected. */
	c->socket = -1;

	printf_threadsafe("connecting to user '%s'\n",c->remote_user);

	struct sockaddr_in server_addr;
	struct sockaddr *server_addr_p = (struct sockaddr *)&server_addr;
	bzero(server_addr_p,sizeof(struct sockaddr_in));
	/* look up user details */
	if (!lookup_connection(&server_addr,c->remote_user)) {
		printf_threadsafe("That user's not logged in!\n");
		return;
	}

	/* create socket */
	if ((c->socket = socket(AF_INET,SOCK_STREAM,0)) < 0) {
		error(0,errno,"socket creation failed");
		return;
	}

	/* connect to the server */
	if (connect(c->socket,server_addr_p,sizeof(struct sockaddr_in)) < 0) {
		error(0,errno,"could not connect to server");
		c->socket = -1;
		return;
	}
	
	pthread_t receive_messages_thread;
	pthread_create(&receive_messages_thread,
		       NULL,
		       receive_messages,
		       c);
	return;
}

int send_message(struct connection *c, char message[INPUT_LEN]) {
	int len = sizeof(char)*INPUT_LEN;
	if (c->socket < 0) {
		printf_threadsafe("You're not connected to anyone!\n");
	} else {
		if (send(c->socket,message,len,0) < len) {
			/* non-fatal error */
			error(0,errno,"failed to send message.");
		}
	}
	/* connection is only detected closed when calling recv */
	return 1;
}
