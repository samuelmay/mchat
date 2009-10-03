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
#include <unistd.h>
#include <errno.h>
#include <error.h>
#include <pthread.h>

#include "user.h"
#include "options.h"
#include "console.h"
#include "connection.h"

void connect_user(char remote_user[USERNAME_LEN]) {
	int s;
	int i;
	struct sockaddr_in server_addr;
	struct sockaddr *server_addr_p = (struct sockaddr *)&server_addr;
	char ack[MESSAGE_LEN];
	int cancel = 1;		/* we set this to zero if we have a successful
				 * outcome. */

	bzero(server_addr_p,sizeof(struct sockaddr_in));

        /* UNSAFE CONCURRENT STUFF BEGINS */	
	pthread_mutex_lock(&user_list_lock);
	i = lookup_user(remote_user);
	if (i < 0) {
		printf_threadsafe("that user's not logged in!\n\n");
	} else if (user_list[i].socket != 0) {
		printf_threadsafe("you're already connected to that user!\n\n");
	} else {
		server_addr.sin_family = AF_INET;
		server_addr.sin_port = user_list[i].port;
		server_addr.sin_addr.s_addr = user_list[i].ip;
		/* SO MUCH GORRAM ERROR HANDLING, INVENT EXCEPTIONS ALREADY */
		if ((s = socket(AF_INET,SOCK_STREAM,0)) < 0 ||
		    connect(s,server_addr_p,sizeof(struct sockaddr_in)) < 0) {
			error(0,errno,"could not connect to server");
		} else if (send(s,opts.username,USERNAME_LEN,0)
			   < USERNAME_LEN) {
			error(0,errno,
			      "failed to send username to open new connection");
		} else if (recv(s,ack,MESSAGE_LEN,0)
			   < MESSAGE_LEN) {
			error(0,errno,"failed to recieve ack");
		} else if (strncmp(ack,"UNKNOWN",MESSAGE_LEN) == 0) {
			printf_threadsafe("%s says they don't know you.\n\n",
					  remote_user);
		} else if (strncmp(ack,"BLOCKED",MESSAGE_LEN) == 0) {
			printf_threadsafe("%s has blocked you!\n\n",
					  remote_user);
		} else if (strncmp(ack,"HI",MESSAGE_LEN) == 0) {
			printf_threadsafe("connecting to user %s\n\n",remote_user);
			/* enter our new connection! */
			user_list[i].flags |= USER_CONNECTED;
			user_list[i].socket = s;
			cancel = 0;
		} else {
			error(0,0,"unknown ack recieved.");
		}
	}
	pthread_mutex_unlock(&user_list_lock);
	/* UNSAFE CONCURRENT STUFF ENDS */

	if (cancel) {
		close(s);
	}

	return;
}


void broadcast_message(char message[INPUT_LEN]) {
	int i;
        /* UNSAFE CONCURRENT STUFF BEGINS */	
	pthread_mutex_lock(&user_list_lock);
	for (i = 0; i < num_users; i++) {
		if (user_list[i].flags & USER_CONNECTED) {
			/* first send our username, then send the message */
			if (send(user_list[i].socket,
				 opts.username,
				 USERNAME_LEN,
				 0) < USERNAME_LEN ||
			    send(user_list[i].socket,
				 message,
				 INPUT_LEN,
				 0) < INPUT_LEN) {
				error(0,errno,"failed to send message.");
			}
		}
	}			    
	pthread_mutex_unlock(&user_list_lock);
	/* UNSAFE CONCURRENT STUFF ENDS */
	return;
}
