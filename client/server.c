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
#include <unistd.h>
#include <errno.h>
#include <error.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/time.h>

#include "user.h"
#include "options.h"
#include "console.h"
#include "connection.h"

int start_listening(struct options *opts) {
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
	
	if (listen(s,10) < 0) {
		error(EXIT_FAILURE,errno,"failed to listen on listening socket");
	}
	
	return s;
}


void *server_thread(void *arg) {
	int server_socket = *((int *)arg);
	int i;
	fd_set fds;
	int max_fd;
	struct timeval timeout;

	/* start our main 'select' loop */
	while (1) {
		/* build up fd list from user list */
		FD_ZERO(&fds);
		FD_SET(server_socket,&fds);
		max_fd = server_socket;
		/* BEGIN UNSAFE CONCURRENT STUFF */
		pthread_mutex_lock(&user_list_lock); 		/* NUMBER 1 */
		for (i = 0; i < num_users; i++) {
			if (user_list[i].socket != 0) {
				FD_SET(user_list[i].socket,&fds); 
				if (user_list[i].socket > max_fd) {
					max_fd = user_list[i].socket;
				}
			}
		}
		pthread_mutex_unlock(&user_list_lock);
		/* END UNSAFE CONCURRENT STUFF */

		/* update the fd listening set every second. It's important to have this
		 * timeout, otherwise the list will never be updated with new outgoing
		 * connections made by client_connect(). We need to set this every time
		 * select() is called, as it is modified on success. */
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

		/* poll! */
		if (select(max_fd+1,&fds,NULL,NULL,&timeout) == -1) {
			error(0,errno,"select failed!");
		}

		/* look to see which socket was activated */
		for (i = 0; i <= max_fd; i++) {
			if (FD_ISSET(i,&fds)) {
				if (i == server_socket) {
					accept_new_connection(i);
				} else {
					receive_message(i);
				}
			}
		}
	}

	return NULL;
}

void accept_new_connection(int fd) {
	int s;
	int retval;
	int i;
	char remote_user[USERNAME_LEN];
	struct sockaddr_in client_addr;
	struct sockaddr *client_addr_p = (struct sockaddr *)&client_addr;
	socklen_t client_addr_len = sizeof(struct sockaddr_in);

	bzero(client_addr_p,sizeof(struct sockaddr_in));
	if ((s = accept(fd,client_addr_p,&client_addr_len)) < 0) {
		error(0,errno,"failed to accept connection");
		return;
	}

	/* they should send us their username */
	retval = recv(s,remote_user,USERNAME_LEN,0);
	if (retval == 0) {
		error(0,errno,"connection was closed as soon as it was opened. WTF?");
		close(s);
		return;
	} else if (retval < USERNAME_LEN) {
		error(0,errno,"failed to receive username from new connection.");
		close(s);
		return;
	}

	/* reverse look up user details to get username */
	/* UNSAFE CONCURRENT STUFF BEGINS */
	pthread_mutex_lock(&user_list_lock);
	if ((i = lookup_user(remote_user)) < 0) {
		error(0,0,"received connection attempt from unknown user");
		close(s);
	} else {
		user_list[i].socket = s; 
		printf_threadsafe("\naccepted incoming connection from %s.\n",
			remote_user);
	}
	pthread_mutex_unlock(&user_list_lock);
	/* UNSAFE CONCURRENT STUFF ENDS */
	return;
}

void receive_message(int fd) {
	char remote_user[USERNAME_LEN];
	char buffer[INPUT_LEN];
	int i;
	int retval;

	if ((retval = recv(fd,remote_user,USERNAME_LEN,0)) < USERNAME_LEN && 
	    retval != 0) {
		/* short read on username (message header) */
		error(0,errno,"failed to recieve username for incoming message");
	} else if ((retval = recv(fd,buffer,INPUT_LEN,0)) < INPUT_LEN &&
		   retval != 0) {
		/* short read on message */
		error(0,errno,"failed to receive message.");
	} else if (retval == 0) {
                /* closed connection at remote end. close socket and remove from
                 * user list. */
		printf_threadsafe("\n%s closed the connection.\n", remote_user); 
		/* UNSAFE CONCURRENT STUFF BEGINS */
		pthread_mutex_lock(&user_list_lock);
		/* we don't care if they're not on the user list, seeing as we're
		 * deleting them anyway. */
		if ((i = lookup_user(remote_user)) >= 0) {
			user_list[i].socket = 0; 
		}
		close(fd);
		pthread_mutex_unlock(&user_list_lock);
		/* UNSAFE CONCURRENT STUFF ENDS */
	} else {
		/* Everything is good! Print out the recieved message and the
		 * user that sent it. */
		printf_threadsafe("\n%s says: %s",remote_user,buffer); 
	} 

	return;
}
