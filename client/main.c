/*
 * chatClient.c
 *
 * Sam May 3206842
 * 22/09/09
 *
 * Client/server for a simple UDP internet chat protocol. A TELE3118
 * mini-project.
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
#include <unistd.h>
#include <pthread.h>
#include "chat.h"

pthread_mutex_t console_lock = PTHREAD_MUTEX_INITIALIZER;

int main (int argc, char **argv) {
	int connection_socket = -1;

	/* get server connection details from command-line */
	struct server_options server_opts;
	bzero(&server_opts,sizeof(struct server_options));
	parse_cmdline(argc,argv,&server_opts);

	printf("Welcome to Sam's chat client, %s. "
	       "Connecting to %s on port %hu.\n",
	       server_opts.username,
	       server_opts.ip_string,
	       server_opts.port_hostformat);

	/* create a background thread to stay in touch with the server */
	pthread_t server_polling_thread;
	pthread_create(&server_polling_thread,
		       NULL,
		       poll_server,
		       &server_opts);

	/* loop on the input prompt, waiting for commands */
	char input[INPUT_LEN];
	char remote_user [USERNAME_LEN];
	while (1) {
		bzero(input,INPUT_LEN);
		fgets(input,INPUT_LEN,stdin);
		if (strncmp(input,"/list\n",INPUT_LEN) == 0) {
			print_user_list();
		} else if (strncmp(input,"/talk",5) == 0) {
			/* Initialize a TCP connection with a given user. */
			sscanf(input,"/talk %13s\n",remote_user);
			connection_socket = client_connect(remote_user);
			/* the client should send the first message, make sure
			 * we don't wait for one the first time */
			continue;
		} else if (strncmp(input,"/listen\n",INPUT_LEN) == 0) {
			/* Wait for a TCP connection from another user. */
			connection_socket = server_accept(remote_user);
		} else if (strncmp(input,"/quit\n",INPUT_LEN) == 0 ||
			   strncmp(input,"/bye\n",INPUT_LEN) == 0) {
			break;
		} else {
			send_message(connection_socket,input);
		}

		if (connection_socket > 0) {
			bzero(input,INPUT_LEN);
			recieve_message(connection_socket,input);
			printf("message from %s: %s",remote_user,input);
		} 
	} 
	printf("Bye.\n");
	return 0;
}	

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
