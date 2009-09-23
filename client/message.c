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

void send_message(struct connection *c, char message[]) {
	int length = strlen(message);
	if (c->socket < 0) {
		printf_threadsafe("You're not connected to anyone!\n");
	} else {
		if (send(c->socket,message,length,0) < 0) {
			/* non-fatal error */
			error(0,errno,"failed to send message.");
		}
	}
	return;
}

void receive_message(struct connection *c,char message[INPUT_LEN]) {
	if (c->socket < 0) {
		printf_threadsafe("You're not connected to anyone!\n");
	} else {
		if (recv(c->socket,message,INPUT_LEN,0) < 0) {
			error(0,errno,"failed to receive message.");
		}
		printf_threadsafe("%s says: %s\n",c->remote_user,message);
	}
	return;
}

void *receive_messages(void *arg) {
	struct connection *c = (struct connection *)arg;
	char buffer[INPUT_LEN];
	while (1) {
		receive_message(c,buffer);
		printf_threadsafe("message from %s: %s\n",c->remote_user,buffer);
	}
	return NULL;
}
