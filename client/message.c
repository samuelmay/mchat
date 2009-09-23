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
#include "chat.h"

void send_message(int socket, char message[]) {
	int length = strlen(message);
	if (socket < 0) {
		printf("You're not connected to anyone!\n");
	} else {
		if (send(socket,message,length,0) < 0) {
			/* non-fatal error */
			error(0,errno,"failed to send message.");
		}
	}
	return;
}

void recieve_message(int socket,char message[]) {
	if (socket < 0) {
		printf("You're not connected to anyone!\n");
	} else {
		if (recv(socket,message,INPUT_LEN,0) < 0) {
			error(0,errno,"failed to recieve message.");
		}
	}
	return;
}
