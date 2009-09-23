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

	struct connection con;
	con.socket = -1;

	/* loop on the input prompt, waiting for commands */
	char input[INPUT_LEN];
	while (1) {
		bzero(input,INPUT_LEN*sizeof(char));
		printf("> ");
		fgets(input,INPUT_LEN,stdin);
		if (strncmp(input,"list\n",INPUT_LEN) == 0) {
			print_user_list();
		} else if (strncmp(input,"connect ",8) == 0 &&
			   sscanf(input,"connect %13s\n",con.remote_user) == 1) {
			/* Initialize a TCP connection with a given user. */ 
			client_connect(&con);
		} else if (strncmp(input,"listen\n",INPUT_LEN) == 0) {
			/* Wait for a TCP connection from another user. */
			server_accept(&con);
			receive_message(&con,input);
		} else if (strncmp(input,"quit\n",INPUT_LEN) == 0 ||
			   strncmp(input,"bye\n",INPUT_LEN) == 0) {
			break;
		} else if (strncmp(input,"msg ",4) == 0) {
			/* remember to set the specifier length for the above
			 * scanf to INPUT_LEN */
			send_message(&con,&(input[4]));
			bzero(input,INPUT_LEN);
			receive_message(&con,input);
		} else {
			/* the 'ed' school of error reporting. */
			printf_threadsafe("Unknown or invalid command.\n");
		}
	} 
	/* don't care about the errors here. */
	shutdown(con.socket,SHUT_RDWR);
	close(con.socket);

	printf_threadsafe("Bye.\n");
	return 0;
}	

