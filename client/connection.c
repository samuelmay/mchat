/*
 * connection.c
 *
 * Sam May
 * 06/10/09
 *
 * Code managing TCP network connections
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
#include <pthread.h>

#include "user.h"
#include "options.h"
#include "console.h"
#include "connection.h"

int start_listening(struct options *opts) {
	/* create socket and start listening for incoming connections */
	int s;
	if ((s = socket(AF_INET,SOCK_STREAM,0)) < 0) {
		perror("listening socket creation failed");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in server_addr;
	struct sockaddr *server_addr_p = (struct sockaddr *)&server_addr;
	socklen_t len = sizeof(struct sockaddr_in);
	memset(server_addr_p,0,len);

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = opts->local_port;

	if (bind(s,server_addr_p,len) < 0) {
		perror("failed to bind listening socket");
		exit(EXIT_FAILURE);
	}

	/* since we binded to any free port, get the actual port we binded to,
	 * and set the options we will send to the registration server */
	if (getsockname(s,server_addr_p,&len) < 0) {
		perror("could not get port from bind");
		exit(EXIT_FAILURE);
	}
	opts->local_port = server_addr.sin_port;
	opts->local_port_h = ntohs(server_addr.sin_port);

	if (listen(s,10) < 0) {
		perror("failed to listen on listening socket");
		exit(EXIT_FAILURE);
	}

	return s;
}

void accept_new_connection(int fd) {
	int s;
	int i;

	struct sockaddr_in client_addr;
	struct sockaddr *client_addr_p = (struct sockaddr *)&client_addr;
	socklen_t client_addr_len = sizeof(struct sockaddr_in);

	memset(client_addr_p,0,client_addr_len);
	if ((s = accept(fd,client_addr_p,&client_addr_len)) < 0) {
		perror("accept new connection");
		if (close(s) < 0) {
			perror("accept new connection");
		}
		return;
	}

	/* reverse look up user details to get username */
	/* UNSAFE CONCURRENT STUFF BEGINS */
	pthread_mutex_lock(&user_list_lock);

	if ((i = lookup_ip(client_addr.sin_addr.s_addr)) < 0 ||
	    user_list[i].flags & USER_BLOCKED) {
		/* they're not on the user list, or they're not wanted. close connection. */
		if (close(s) < 0) {
			perror("accept new connection");
		}
	} else {
		user_list[i].flags |= USER_CONNECTED; /* set the bit */
		user_list[i].socket = s;
		printf("\naccepted incoming connection from "
		       USERNAME_PRINT_FMT ".",
		       user_list[i].name);
		fflush(stdout);	/* needed to force display if we don't print a
				 * newline */
	}
	pthread_mutex_unlock(&user_list_lock);
	/* UNSAFE CONCURRENT STUFF ENDS */
	return;
}

void receive_message(int fd) {
	char buffer[INPUT_LEN];
	int i;
	int retval;

	if ((retval = recv(fd,buffer,INPUT_LEN,0)) < INPUT_LEN &&
	    retval != 0) {
		/* short read on message */
		perror("failed to receive message.");
	} else if (retval == 0) {
                /* closed connection at remote end. close socket and remove from
                 * user list. */
		/* UNSAFE CONCURRENT STUFF BEGINS */
		pthread_mutex_lock(&user_list_lock);
		/* We don't care if they're not on the user list, seeing as
		 * we're deleting them anyway. */
		if ((i = lookup_socket(fd)) >= 0) {
			user_list[i].flags &= ~USER_CONNECTED;/*clear the bit*/
			user_list[i].socket = 0;
		}

		printf("\n" USERNAME_PRINT_FMT " closed their connection.",
		       user_list[i].name);
		fflush(stdout);

		if (close(fd) < 0) {
			perror("recieve message");
		}
		pthread_mutex_unlock(&user_list_lock);
		/* UNSAFE CONCURRENT STUFF ENDS */
	} else {
		/* Everything is good! Print out the recieved message and the
		 * user that sent it. */

		chomp(buffer,INPUT_LEN);

		pthread_mutex_lock(&user_list_lock);
		if ((i = lookup_socket(fd)) < 0) {
			/* this should never happen, as how would we have an
			 * open connection and socket from an unknown user? */
			perror("message from unknown user");
		} else {
			printf("\n" USERNAME_PRINT_FMT " says: " INPUT_PRINT_FMT,
			       user_list[i].name, buffer);
			fflush(stdout);
		}
		pthread_mutex_unlock(&user_list_lock);
	}

	return;
}

void connect_user(char remote_user[USERNAME_LEN]) {
	int s;
	int i;
	struct sockaddr_in server_addr;
	struct sockaddr *server_addr_p = (struct sockaddr *)&server_addr;

	memset(server_addr_p,0,sizeof(struct sockaddr_in));

        /* UNSAFE CONCURRENT STUFF BEGINS */
	pthread_mutex_lock(&user_list_lock);
	i = lookup_user(remote_user);
	if (i < 0) {
		printf("that user's not logged in!\n");
	} else if (user_list[i].socket != 0) {
		printf("you're already connected to that user!\n");
	} else {
		server_addr.sin_family = AF_INET;
		server_addr.sin_port = user_list[i].port;
		server_addr.sin_addr.s_addr = user_list[i].ip;

		if ((s = socket(AF_INET,SOCK_STREAM,0)) < 0 ||
		    connect(s,server_addr_p,sizeof(struct sockaddr_in)) < 0) {
			perror("could not connect to server");
			if (close(s) < 0) {
				perror("connect user");
			}
		} else {
			printf("connecting to user " USERNAME_PRINT_FMT "\n",
			       remote_user);
			/* enter our new connection! */
			user_list[i].flags |= USER_CONNECTED;
			user_list[i].socket = s;
		}
	}
	pthread_mutex_unlock(&user_list_lock);
	/* UNSAFE CONCURRENT STUFF ENDS */

	return;
}

void disconnect_user(char remote_user[USERNAME_LEN]) {
	int i;
	/* BEGIN UNSAFE CONCURRENT STUFF */
	pthread_mutex_lock(&user_list_lock);
	i = lookup_user(remote_user);
	if (i < 0) {
		printf("that user's not logged in!\n");
	} else if (!(user_list[i].flags & USER_CONNECTED)) {
		printf("you're not connected to that user!\n");
	} else {
		if (close(user_list[i].socket) < 0) {
			perror("disconnect user");
		}
		user_list[i].flags &= ~USER_CONNECTED;
		user_list[i].socket = 0;
		printf(USERNAME_PRINT_FMT " was disconnected.\n",
		       remote_user);
	}
	pthread_mutex_unlock(&user_list_lock);
	/* END UNSAFE CONCURRENT STUFF */
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
				 message,
				 INPUT_LEN,
				 0) < INPUT_LEN) {
				perror("failed to send message");
			}
		}
	}
	pthread_mutex_unlock(&user_list_lock);
	/* UNSAFE CONCURRENT STUFF ENDS */
	return;
}

void send_message(char remote_user[USERNAME_LEN],char message[INPUT_LEN]) {
	int i;
	pthread_mutex_lock(&user_list_lock);
	i = lookup_user(remote_user);
	if (i < 0) {
		printf("that user's not logged in!\n");
	} else if (!(user_list[i].flags & USER_CONNECTED)) {
		printf("you're not connected to that user!\n");
	} else if (send(user_list[i].socket,
			message,
			INPUT_LEN,
			0) < INPUT_LEN) {
		perror("failed to send message");
	}
	pthread_mutex_unlock(&user_list_lock);
	return;
}
