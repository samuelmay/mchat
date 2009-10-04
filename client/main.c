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
#include <unistd.h>
#include <pthread.h>

#include "user.h"
#include "options.h"
#include "console.h"
#include "connection.h"
#include "registration.h"
#include "commands.h"

pthread_mutex_t console_lock = PTHREAD_MUTEX_INITIALIZER;
struct options opts;

int main (int argc, char **argv) {

	/* initialize global user list */
	bzero(&user_list,MAX_USERS*sizeof(struct user));
	num_users = 0;

	/* get registration server connection details from command-line */
	bzero(&opts,sizeof(struct options));
	parse_cmdline(argc,argv,&opts);

	printf("Welcome to Sam's chat client, %s.\n", opts.username);

	/* open local port to listen for incoming connections */
	int server_socket = start_listening(&opts); 
        /* start_listenin() modifies opts to contain the port we were given by
	 * bind() */
	printf("Listening on port %hu.\n",opts.local_port_h); 

	/**** locks are needed once we create threads ****/

	/* create a background thread to stay in touch with the server */
	printf("Connecting to %s on port %hu.\n", 
	       opts.ip_string, 
	       opts.server_port_h);
	pthread_t registration;
	pthread_create(&registration,
		       NULL,
		       registration_thread,
		       &opts);

	/* create a background thread to listen for incoming connections. */
	pthread_t server;
	pthread_create(&server,
		       NULL,
		       server_thread,
		       &server_socket);

	/* loop on the input prompt, waiting for commands */
	char input[INPUT_LEN];
	char arg1[INPUT_LEN];
	while (1) {
		bzero(input,INPUT_LEN*sizeof(char));
		printf_threadsafe("> ");
		fgets(input,INPUT_LEN,stdin);
		if (strncmp(input,"\n",INPUT_LEN) == 0) {
			continue;
		} else if (strncmp(input,"msg ",4) == 0 &&
			   sscanf(input,"msg %255s\n",arg1) == 1) {
			/* remember to set the specifier length for the above
			 * scanf to INPUT_LEN */
			broadcast_message(arg1);
		} else if (strncmp(input,"list\n",5) == 0) {
			print_user_list();
		} else if (strncmp(input,"update\n",7) == 0) {
			pthread_cond_signal(&registration_update);
		} else if (strncmp(input,"connect ",8) == 0 &&
			   sscanf(input,"connect %13s\n",arg1) == 1) {
			/* Initialize a TCP connection with a given user. */ 
			connect_user(arg1);
		} else if (strncmp(input,"disconnect ",11) == 0 &&
			   sscanf(input,"disconnect %13s\n",arg1) == 1) {
			disconnect(arg1);
		} else if (strncmp(input,"block ",6) == 0 &&
			   sscanf(input,"block %13s\n",arg1) == 1) {
			block(arg1);
		} else if (strncmp(input,"unblock ",8) == 0 &&
			   sscanf(input,"unblock %13s\n",arg1) == 1) {
			unblock(arg1);
		} else if (strncmp(input,"help",4) == 0) {
			help();
		} else if (strncmp(input,"quit\n",5) == 0 ||
			   strncmp(input,"bye\n",4) == 0 || 
			   strlen(input) == 0) {
			/* if the length of the input is 0, someone hit
			 * Ctrl-D */
			break;
		} else {
			/* the 'ed' school of error reporting. */
			printf_threadsafe("Unknown or invalid command.\n");
		}
	} 

	/* stop all threads */
	pthread_cancel(server);
	pthread_cancel(registration);
	/* shutdown all the open sockets */
	int i;
	for (i = 0; i < num_users; i++) {
		if (user_list[i].socket != 0) {
			close(user_list[i].socket);
		}
	}

	printf("Bye.\n");
	return 0;
}	

