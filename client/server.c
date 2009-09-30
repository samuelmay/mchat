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


void *server_thread(void *arg) {
	int server_socket = *((int *)arg);
	char remote_user[USERNAME_LEN];
	char buffer[INPUT_LEN];
	int s;
	int retval;
	int len = sizeof(char)*INPUT_LEN;

	struct sockaddr_in client_addr;
	struct sockaddr *client_addr_p = (struct sockaddr *)&client_addr;

	socklen_t client_addr_len = sizeof(struct sockaddr_in);

	bzero(client_addr_p,sizeof(struct sockaddr_in));
	if ((s = accept(server_socket,client_addr_p,&client_addr_len)) < 0) {
		error(0,errno,"failed to accept connection");
		server_socket = -1;
		return NULL;
	} else {
		/* use the newly created socket */
		server_socket = s;
	}

	/* reverse look up user details to get username */
	if (!lookup_user(&client_addr,remote_user)) {
		strncpy(remote_user,"unknown",USERNAME_LEN);
	}
		
	printf_threadsafe("\naccepted incoming connection from %s.\n",remote_user);
		
	while (1) {
		retval = recv(server_socket,buffer,len,0);
		if (retval == 0) {
			/* closed connection at remote end */
			break;
		} else if (retval < len) {
			error(0,errno,"failed to receive message.");
		}
		printf_threadsafe("\n%s says: %s",remote_user,buffer);
			
		bzero(buffer,INPUT_LEN*sizeof(char));
	}
	printf_threadsafe("\n%s closed the connection.\n", remote_user); 
	return NULL;
}
