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
		printf("> ");
		fgets(input,INPUT_LEN,stdin);
		if (strncmp(input,"list\n",INPUT_LEN) == 0) {
			print_user_list();
		} else if (strncmp(input,"connect ",8) == 0 &&
			   sscanf(input,"connect %13s\n",remote_user) == 1) {
			/* Initialize a TCP connection with a given user. */ 
			connection_socket = client_connect(remote_user);
		} else if (strncmp(input,"listen\n",INPUT_LEN) == 0) {
			/* Wait for a TCP connection from another user. */
			connection_socket = server_accept(remote_user);
		} else if (strncmp(input,"quit\n",INPUT_LEN) == 0 ||
			   strncmp(input,"bye\n",INPUT_LEN) == 0) {
			break;
		} else if (strncmp(input,"msg ",4) == 0) {
			/* remember to set the specifier length for the above
			 * scanf to INPUT_LEN */
			send_message(connection_socket,&(input[5]));
			bzero(input,INPUT_LEN);
			recieve_message(connection_socket,input);
			printf("message from %s: %s", remote_user,input);
		} else {
		/* the 'ed' school of error reporting. */
			printf("Unknown or invalid command.\n");
		}
	} 
	/* don't care about the errors here. */
	shutdown(connection_socket,SHUT_RDWR);
	close(connection_socket);

	printf("Bye.\n");
	return 0;
}	

