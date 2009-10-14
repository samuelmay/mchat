/*
 * chatClient.c
 *
 * Sam May 3206842
 * 22/09/09
 *
 * Client/server for a simple internet chat protocol. A TELE3118 mini-project.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
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
	memset(&user_list,0,MAX_USERS*sizeof(struct user));
	num_users = 0;

	/* get registration server connection details from command-line */
	memset(&opts,0,sizeof(struct options));
	parse_cmdline(argc,argv,&opts);

	printf("Welcome to Sam's chat client, " USERNAME_PRINT_FMT ".\n",
	       opts.username);

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

	/**** main loop ****/

	char input[INPUT_LEN];
	int i;
	int quit = 0;
	fd_set fds;
	int max_fd;

	printf("> ");
	fflush(stdout);

	while (!quit) {
		/* build up fd list from user list */
		FD_ZERO(&fds);
		FD_SET(server_socket,&fds); /* for incoming connections */
		FD_SET(STDIN_FILENO,&fds);  /* for user input */
		max_fd = server_socket;
		/* BEGIN UNSAFE CONCURRENT STUFF */
		pthread_mutex_lock(&user_list_lock); 		/* NUMBER 1 */
		for (i = 0; i < num_users; i++) {
			if (user_list[i].flags & USER_CONNECTED) {
				FD_SET(user_list[i].socket,&fds);
				if (user_list[i].socket > max_fd) {
					max_fd = user_list[i].socket;
				}
			}

		}
		pthread_mutex_unlock(&user_list_lock);
		/* END UNSAFE CONCURRENT STUFF */

		/* poll! */
		if (select(max_fd+1,&fds,NULL,NULL,NULL) == -1) {
			perror("select failed");
		}

		/* look to see which socket was activated */
		for (i = 0; i <= max_fd; i++) {
			if (FD_ISSET(i,&fds)) {
				if (i == STDIN_FILENO) {
					memset(input,0,INPUT_LEN*sizeof(char));
					fgets(input,INPUT_LEN,stdin);
					quit = execute_command(input);
				} else if (i == server_socket) {
					accept_new_connection(i);
				} else {
					receive_message(i);
				}
			}
		}
	}

	/* Stop all threads. Don't care about errors and return values at this
	 * point really. */
	pthread_cancel(registration);
	/* Shutdown all the open sockets. */
	for (i = 0; i < num_users; i++) {
		if (user_list[i].socket != 0) {
			close(user_list[i].socket);
		}
	}

	printf("Bye.\n");
	return 0;
}

/* c.f. Perl function of the same name. If last character in a string is a
 * newline, remove it. */
void chomp(char *string, size_t maxlen) {
	/* surprisingly, strnlen is not portable to BSD (i.e. OS X). Do
	 * it by hand */
	/* i = strnlen(buffer,maxlen); */
	char *p = memchr(string,0,maxlen);
	int i = p ? p - string : maxlen;

	if (string[i-1] == '\n') {
		string[i-1] = '\0';
	}
	return;
}
