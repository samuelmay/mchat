/*
 * message.c
 *
 * Sam May
 * 23/09/09
 *
 * Functions for recieving and sending chat messages over a TCP socket
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
