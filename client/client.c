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

#include "user.h"
#include "options.h"
#include "console.h"
#include "connection.h"

void client_connect(char remote_user[USERNAME_LEN]) {
	int s;
	int i;
	struct sockaddr_in server_addr;
	struct sockaddr *server_addr_p = (struct sockaddr *)&server_addr;

	bzero(server_addr_p,sizeof(struct sockaddr_in));

        /* UNSAFE CONCURRENT STUFF BEGINS */	
	pthread_mutex_lock(&user_list_lock);
	i = lookup_user(remote_user);
	if (i < 0) {
		printf_threadsafe("that user's not logged in!\n");
	} else if (user_list[i].socket != 0) {
		printf_threadsafe("you're already connected to that user!\n");
		i = -1;
	} else {
		printf_threadsafe("connecting to user '%s'\n",remote_user);
		server_addr.sin_family = AF_INET;
		server_addr.sin_port = user_list[i].port;
		server_addr.sin_addr.s_addr = user_list[i].ip;

		if ((s = socket(AF_INET,SOCK_STREAM,0)) < 0 ||
		    connect(s,server_addr_p,sizeof(struct sockaddr_in)) < 0) {
			error(0,errno,"could not connect to server");
		} else {
			/* enter our new connection! */
			user_list[i].socket = s;
		}
	}
	pthread_mutex_unlock(&user_list_lock);
	/* UNSAFE CONCURRENT STUFF ENDS */

	/* as first order of business, send our username */
	if (send(s,opts.username,USERNAME_LEN,0) < USERNAME_LEN) {
		error(0,errno,
		      "failed to send username to open new connection");
	}
	return;
}


void broadcast_message(char message[INPUT_LEN]) {
	int i;
        /* UNSAFE CONCURRENT STUFF BEGINS */	
	pthread_mutex_lock(&user_list_lock);
	for (i = 0; i < num_users; i++) {
		if (user_list[i].socket != 0) {
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
